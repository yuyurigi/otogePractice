#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    //アイコンの設定
    setSvg();
    
    //NanoVG
    canvas.allocate(ofGetWidth(), ofGetHeight());
    pink = ofFloatColor(238/255.0f, 126/255.0f, 177/255.0f);
    deepPink = ofFloatColor(254/255.0f, 0/255.0f, 142/255.0f);
    white = ofFloatColor(1, 1, 1);
    lightBlue = ofFloatColor(106/255.0f, 196/255.0f, 218/255.0f);
    yellow = ofFloatColor(247/255.0f, 204/255.0f, 70/255.0f);
    assert(canvas.loadFont("Corporate-Logo-Rounded.ttf", "corpo"));
    
    //Parameter
    bPause = false; //ビデオの一時停止（trueだと一時停止する）
    speed = 1.0; //ビデオの再生スピード
    bSpeedBar = false; //スピード調整用のバーを表示（false:非表示）
    bRepeat = true; //ビデオのループ設定
    
    textPos = glm::vec2(100, ofGetHeight()-255+35); //テキスト（現在の再生時間）の位置
    textSize = 40; //[現在の再生時間、トータルの再生時間]のテキストサイズ
    ofxNanoVG::Canvas & c = canvas;
    c.textAlign(ofxNanoVG::TextAlign::RIGHT | ofxNanoVG::TextAlign::TOP);
    c.textSize(textSize);
    ofRectangle r = c.textBounds("00:00", ofGetWidth()-textPos.x, textPos.y); //トータルの再生時間のテキストを入れたバウンディングボックス
    c.textSize(70); //再生スピードのテキストサイズ
    ofRectangle sr = c.textBounds("×1.0", 0, 0); //再生スピードのテキストを入れたバウンディングボックス
    barPos = glm::vec2(textPos.x+r.width+20, textPos.y+r.height/2); //シークバーの位置（左）
    text2Y = textPos.y+r.height+20; //テキスト２段目のy位置
    speedR = ofRectangle(ofGetWidth()-textPos.x-sr.width, text2Y, sr.width, sr.height); //再生スピードのテキストの位置（左上）と幅、高さ
    speedBoxR = ofRectangle(speedR.x+70, speedR.y-450, speedR.width-70, 450); //再生スピードのボックス　x, y, w, h
    //70:再生スピードのテキストの位置（左x位置）からどれだけ、ずれるか
    //450:ボックスの高さ
    sBarR = ofRectangle(speedBoxR.x+speedBoxR.width/2, speedBoxR.y+40, 1, speedBoxR.height-80); //再生スピードのバー
    c.textSize(30); //説明テキストのテキストサイズ
    c.textAlign(ofxNanoVG::TextAlign::LEFT | ofxNanoVG::TextAlign::TOP);
    ofRectangle directions = c.textBounds("再生", 0, 0);
    directionsPos = glm::vec2(40, ofGetHeight()-10-directions.height); //説明テキストの位置
    
    count = 0;
    
    //ビデオ関連、xmlの設定
    setVideo();

}
//--------------------------------------------------------------
void ofApp::setSvg(){
    //svgフォルダに入ってる.svgファイルをロードする
    vector<ofxSVG> svgs;
    ofDirectory dir;
    dir.listDir("svg/");
    dir.allowExt("svg");
    dir.sort();
    if (dir.size()) {
        svgs.assign(dir.size(), ofxSVG());
    }
    for (int i = 0; i < (int)dir.size(); i++) {
        svgs[i].load(dir.getPath(i));
    }
    svg = ofRectangle(0, 0, svgs[0].getWidth(), svgs[0].getHeight());
    
    icons.resize(svgs.size());
    //svgをofPathの配列（icons）に代入
    int num = 0;
    for (int i = 0; i < svgs.size(); i++) {
        for (ofPath p: svgs[i].getPaths()){
            p.setPolyWindingMode(OF_POLY_WINDING_ODD);
            const vector<ofPolyline>& lines = p.getOutline();
            vector<ofPolyline> outlines;
            for(const ofPolyline & line: lines){
                outlines.push_back(line.getResampledBySpacing(1));
            }
            for (int j = 0; j < outlines.size(); j++) {
                ofPolyline & line = outlines[j];
                for (int k = 0; k < line.size(); k++) {
                    if (j==0 && k==0){
                        icons[num].lineTo(line[k]);
                    } else if(j>0 && k==0){
                        icons[num].moveTo(line[k]);
                    }else if (k > 0) {
                        icons[num].lineTo(line[k]);
                    }
                }
                icons[num].close();
            }
        }
        num += 1;
    }
    
    for (int i = 0; i < icons.size(); i++) {
        icons[i].setColor(ofColor(255)); //アイコンの色
        icons[i].setPolyWindingMode(OF_POLY_WINDING_ODD);
    }
}
//--------------------------------------------------------------
void ofApp::setVideo(){
    //xml
    if(!xml.load("parameter.xml")){
        ofLogError() << "Couldn't load file";
    }
    drawing = xml.getChild("drawing");
    if(!drawing){ //xmlにdrawingがない場合は新しく追加する
        drawing = xml.appendChild("drawing");
    }
    
    auto fav = drawing.findFirst("fav"); //xmlからfavタグを探す
    if (!fav) { //↑ない場合、xmlに追加する
        drawing.removeChild("fav");
        drawing.appendChild("fav").set(0);
    }
    
    favorite = drawing.getChild("fav").getIntValue(); //favorite : 星をつけたビデオ（xmlから取得）
    currentVideo = favorite; //現在のビデオ
    padding = 30; //動画名のテキストが入ったボックスと文字の間の余白
    videoNameTextSize = 19; //動画名のテキストのサイズ
    font.load("Corporate-Logo-Rounded.ttf", videoNameTextSize);
    
    //videoフォルダに入ってる.movファイルをロードする
    ofDirectory dir;
    dir.listDir("video/");
    dir.sort();
    if (dir.size()) {
        videos.assign(dir.size(), ofVideoPlayer());
    }
    
    videoName.resize(dir.size());
    videoNameR.resize(dir.size());
    bStarMouseOver.resize(dir.size());
    bVideoNameMouseOver.resize(dir.size());
    scrollTime.resize(dir.size());
    for (int i = 0; i < (int)dir.size(); i++) {
        videos[i].load(dir.getPath(i)); //動画をロード
        
        string fileInfo0 = dir.getName(i); //動画ファイルの名前を取得
        string fileInfo1 = ofUTF8Substring(fileInfo0, 0, fileInfo0.size()-4); //うしろの.movを消す
        string fileInfo = convertVideoTitle(fileInfo1); //日本語表記にしたい動画名を変換する
        videoName[i] = fileInfo; //配列に動画名を入れる
        videoNameR[i] = font.getStringBoundingBox(fileInfo, 0, i*25*1.6);
        
        bStarMouseOver[i] = false; //星にマウスオーバーしてるかどうか
        bVideoNameMouseOver[i] = false; //動画名のテキストにマウスオーバーしてるかどうか
        scrollTime[i] = 0; //文字数が多い動画名を画面に表示したときスクロールするための変数
    }
    
    videos[currentVideo].play(); //Start the video to play
    
    //ビデオボックス
    //ビデオボックスのx, y, w, h
    float h = videoNameR[videos.size()-1].y - videoNameR[0].y + videoNameR[0].getHeight()+padding*2;
    videoBoxR = ofRectangle(60, text2Y-15-h, 350, h);
    bVideoBox = false; //ボックス表示用のbool
    
    //ビデオのトータルフレーム数を配列に代入
    totalFrame.resize(videos.size());
    totalTimeText.resize(videos.size());
    for (int i = 0; i < videos.size(); i++) {
        totalFrame[i] = videos[i].getTotalNumFrames();
        
        //ビデオのトータル再生時間を配列（totalTimeText）に入れる
        int totalTime = totalFrame[i]/60;
        int totalSecond = totalTime%60;
        int totalMinute = totalTime/60;
        //秒が１０以下の場合には左に０をつける
        string ts, tm;
        if (totalSecond < 10) {
            ts = "0" + ofToString(totalSecond, 0);
        } else {
            ts = ofToString(totalSecond, 0);
        }
        //分が１０以下の場合には左に０をつける
        if (totalMinute < 10) {
            tm = "0" + ofToString(totalMinute, 0);
        } else {
            tm = ofToString(totalMinute, 0);
        }
        totalTimeText[i] = tm +":"+ ts;
    }
    
    //ブックマーク
    //現在の動画のブックマークをxmlから読み込み
    bookmarks.resize(2);
    string str = removeSpaces(videoName[currentVideo]); //動画名にスペースや！が入ってる場合、削除する
    string str2 = "//"+str;
    auto posXml = xml.findFirst(str2); //動画名と一致するタグがxml内にあるか検索
    if (!posXml) { //↑なければ新しく追加
        drawing.removeChild(str);
        auto bm = drawing.appendChild(str);
        bm.appendChild("pt").set(barPos.x);
        bm.appendChild("pt").set(ofGetWidth()-barPos.x);
        
        bookmarks[0] = glm::vec2(barPos.x, 0); //y 0:通常表示 1:マウスオーバー 2:クリックして選択中
        bookmarks[1] = glm::vec2(ofGetWidth()-barPos.x, 0);
    } else {
        //xmlにデータがある場合はこっちから取得
        auto pts = posXml.getChildren("pt");
        int j = 0;
        for (auto & pt: pts) {
            auto x = pt.getFloatValue();
            if(j < 2){ //配列を最初に２個分だけ確保してるので、３つ以上データがある場合はpush_backで追加する
                bookmarks[j] = glm::vec2(x, 0);
            } else {
                bookmarks.push_back(glm::vec2(x, 0));
            }
            j++;
        }
    }
    
}
//--------------------------------------------------------------
string ofApp::convertVideoTitle(const string &str){
    //指定した動画ファイルを日本語表記に変える
    string str2;
    if(str == "gamingEverything"){
        str2 = regex_replace(str, regex("gamingEverything"), "ゲーミング☆Everything");
    } else if(str == "Funkotsu Saishin Casino"){
        str2 = regex_replace(str, regex("Funkotsu Saishin Casino"), "粉骨砕身カジノゥ");
    } else if(str == "Neonlights"){
        str2 = regex_replace(str, regex("Neonlights"), "ネオンライト");
    } else if (str == "Say! Fanfare!"){
        str2 = regex_replace(str, regex("Say! Fanfare!"), "Say!ファンファーレ!");
    } else {
        str2 = str;
    }
    return str2;
}
//--------------------------------------------------------------
string ofApp::removeSpaces(const string& s) {
    string tmp(s);
    
    //文字列にスペースがある場合はスペースを削除
    int space = tmp.find(" ");
    if (space != std::string::npos){
        tmp.erase(std::remove(tmp.begin(), tmp.end(), ' '), tmp.end());
    }
    
    //文字列に!がある場合は!を削除
    int exclamation = tmp.find("!");
    if (exclamation != std::string::npos){
        tmp.erase(std::remove(tmp.begin(), tmp.end(), '!'), tmp.end());
    }
    
    return tmp;
}
//--------------------------------------------------------------
void ofApp::update(){
    videos[currentVideo].update(); //Decode the new frame if needed
    
    //ビデオが一番最後までいったらカウントをリセット
    if (currentFrame == totalFrame[0]-1) {
        if(bRepeat){
            count = 0;
        } else { //ノーリピートの場合は一時停止アイコンを再生アイコンに変える
            bPause = true;
        }
    }
    
    if(count == 0){
        if (videos[currentVideo].isFrameNew()) {
            currentFrame = videos[currentVideo].getCurrentFrame();
            currentPosX = ofMap(currentFrame, 0, totalFrame[currentVideo], barPos.x, ofGetWidth()-barPos.x);
            count++;
        } else {
            currentFrame = 0;
            currentPosX = barPos.x;
        }
    } else {
        currentFrame = videos[currentVideo].getCurrentFrame();
        //一時停止してる状態でビデオの再生位置をいじるとばぐるので、ばぐらないように前のフレームの数値を取得
        if(currentFrame<0){
            currentFrame = pre_currentFrame;
        }
        currentPosX = ofMap(currentFrame, 0, totalFrame[currentVideo], barPos.x, ofGetWidth()-barPos.x);
    }
    
    //現在の再生時間を計算
    int currentTime = currentFrame/60;
    int currentSecond = currentTime%60;
    int currentMinute = currentTime/60;
    string s, m;
    //秒が１０以下の場合には左に０をつける
    if (currentSecond < 10) {
        s = "0" + ofToString(currentSecond, 0);
    } else {
        s = ofToString(currentSecond, 0);
    }
    //分が１０以下の場合には左に０をつける
    if (currentMinute < 10) {
        m = "0" + ofToString(currentMinute, 0);
    } else {
        m = ofToString(currentMinute, 0);
    }
    text = m + ":" + s;
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofSetColor(255, 255, 255);
    ofxNanoVG::Canvas& c = canvas;
    c.background(1, 1, 1); //float color
    
    c.begin();
    
    //ビデオ
    videos[currentVideo].draw(0, 0, ofGetWidth(), ofGetHeight());
    //ピンクの角丸四角
    c.fillColor(pink);
    c.beginPath();
    c.roundedRect(40, ofGetHeight()-255, ofGetWidth()-40*2, 200, 10); //x, y, w, h, r
    c.fillPath();
    
    //白い線だけの角丸四角
    c.strokeColor(white);
    c.lineWidth(5);
    c.beginPath();
    c.roundedRect(40, ofGetHeight()-255, ofGetWidth()-40*2, 200, 10);
    c.strokePath();
    
    canvas.textFont("corpo");
    //テキスト　現在の再生時間
    c.textSize(textSize);
    c.fillColor(white);
    c.textAlign(ofxNanoVG::TextAlign::LEFT | ofxNanoVG::TextAlign::TOP);
    c.text(text, textPos.x, textPos.y);
    
    //テキスト　動画のトータル再生時間
    c.textAlign(ofxNanoVG::TextAlign::RIGHT | ofxNanoVG::TextAlign::TOP);
    c.text(totalTimeText[currentVideo], ofGetWidth()-textPos.x, textPos.y);
    
    //シークバー（白）
    c.lineCap(ofxNanoVG::LineCap::ROUND);
    c.lineWidth(18);
    c.beginPath();
    c.moveTo(barPos.x, barPos.y);
    c.lineTo(ofGetWidth()-barPos.x, barPos.y);
    c.strokePath();
    
    //シークバー（水色）
    c.strokeColor(lightBlue);
    c.lineWidth(8);
    c.beginPath();
    c.moveTo(barPos.x, barPos.y);
    c.lineTo(currentPosX, barPos.y);
    c.strokePath();
    
    //シークバー　丸
    c.fillColor(lightBlue); //fill
    c.beginPath();
    c.ellipse(currentPosX, barPos.y, 10, 10);
    c.fillPath();
    c.strokeColor(white); //stroke
    c.lineWidth(5);
    c.beginPath();
    c.ellipse(currentPosX, barPos.y, 10, 10);
    c.strokePath();
    
    //テキスト（再生スピード）
    c.fillColor(white); //fill
    c.textSize(70);
    c.textAlign(ofxNanoVG::TextAlign::LEFT | ofxNanoVG::TextAlign::TOP);
    c.text("×"+ofToString(speed, 1), speedR.x, speedR.y);
    
    //再生スピードの調整用バーとボックス
    if (bSpeedBar) {
        c.fillColor(pink); //box(fill)
        c.beginPath();
        c.roundedRect(speedBoxR.x, speedBoxR.y, speedBoxR.width, speedBoxR.height, 10);
        c.fillPath();
        c.lineWidth(5); //box(stroke)
        c.strokeColor(white);
        c.beginPath();
        c.roundedRect(speedBoxR.x, speedBoxR.y, speedBoxR.width, speedBoxR.height, 10);
        c.strokePath();
        c.lineWidth(10);  //bar
        c.beginPath();
        c.moveTo(sBarR.x, sBarR.y);
        c.lineTo(sBarR.x, sBarR.y+sBarR.height);
        c.strokePath();
        c.fillColor(lightBlue); //ellipse
        c.beginPath();
        float speedBarPos = ofMap(speed, 0.3, 1.0, sBarR.y+sBarR.height, sBarR.y);
        c.ellipse(sBarR.x, speedBarPos, 10, 10);
        c.fillPath();
        c.lineWidth(5);
        c.beginPath();
        c.ellipse(sBarR.x, speedBarPos, 10, 10);
        c.strokePath();
    }
    
    //説明テキスト-----------------------------------------------------------------------------------
    //一時停止アイコン、再生アイコンの上にマウスがきたらテキストを表示
    c.textSize(30);
    c.textAlign(ofxNanoVG::TextAlign::LEFT | ofxNanoVG::TextAlign::TOP);
    if (ofGetWidth()/2-45 <= mouseX && mouseX <= ofGetWidth()/2+45 && text2Y+30-45 <= mouseY && mouseY <= text2Y+30+45) {
        ofRectangle r;
        if (bPause) {
            ofRectangle r = c.textBounds("再生(Space)", directionsPos.x, directionsPos.y);
            c.fillColor(deepPink);
            c.beginPath();
            c.rect(directionsPos.x, directionsPos.y, r.width, r.height);
            c.fillPath();
            c.fillColor(white);
            c.text("再生(Space)", directionsPos.x, directionsPos.y);
        } else {
            ofRectangle r = c.textBounds("一時停止(Space)", directionsPos.x, directionsPos.y);
            c.fillColor(deepPink);
            c.beginPath();
            c.rect(directionsPos.x, directionsPos.y, r.width, r.height);
            c.fillPath();
            c.fillColor(white);
            c.text("一時停止(Space)", directionsPos.x, directionsPos.y);
        }
    }
    if(!bVideoBox){
        //シークバーの上にマウスがきたらテキストを表示
        if (barPos.x-5 <= mouseX && mouseX <= ofGetWidth()-barPos.x+5 && barPos.y-9 <= mouseY && mouseY <= barPos.y+9) {
            ofRectangle r = c.textBounds("再生位置を変更　Shiftでブックマークをつけます", directionsPos.x, directionsPos.y);
            c.fillColor(deepPink);
            c.beginPath();
            c.rect(directionsPos.x, directionsPos.y, r.width, r.height);
            c.fillPath();
            c.fillColor(white);
            c.text("再生位置を変更　Shiftでブックマークをつけます", directionsPos.x, directionsPos.y);
        }
        
        //ブックマークの上にマウスがきたらテキストを表示
        if(bookmarks.size()>2){
            for (int i = 1; i < bookmarks.size()-1; i++) {
                if (bookmarks[i].x-25 <= mouseX && mouseX <= bookmarks[i].x+25 && barPos.y-55 <= mouseY && mouseY <= barPos.y-5) {
                    ofRectangle r = c.textBounds("クリックでブックマークを選択　選択後deleteで削除", directionsPos.x, directionsPos.y);
                    c.fillColor(deepPink);
                    c.beginPath();
                    c.rect(directionsPos.x, directionsPos.y, r.width, r.height);
                    c.fillPath();
                    c.fillColor(white);
                    c.text("クリックでブックマークを選択　選択後deleteで削除", directionsPos.x, directionsPos.y);
                    if(bookmarks[i].y!=2) bookmarks[i].y = 1;
                } else {
                    if(bookmarks[i].y!=2) bookmarks[i].y = 0;
                }
            }
        }
    }
    
    //再生スピードの上にマウスがきたらテキストを表示　＆＆　バーの入ったボックスを表示
    if (speedR.x <= mouseX && mouseX <= speedR.x+speedR.width && speedR.y <= mouseY && mouseY <= speedR.y+speedR.height) {
        ofRectangle r = c.textBounds("再生スピード(↑↓)", directionsPos.x, directionsPos.y);
        c.fillColor(deepPink);
        c.beginPath();
        c.rect(directionsPos.x, directionsPos.y, r.width, r.height);
        c.fillPath();
        c.fillColor(white);
        c.text("再生スピード(↑↓)", directionsPos.x, directionsPos.y);
        bSpeedBar = true;
    }
    
    //リピートアイコンの上にマウスがきたらテキストを表示
    if (textPos.x+120<=mouseX && mouseX <= textPos.x+120+90 && text2Y-15 <= mouseY && mouseY <= text2Y+75) {
        if(bRepeat){
            ofRectangle r = c.textBounds("リピートする　クリックまたはR(r)でリピートをオフ", directionsPos.x, directionsPos.y);
            c.fillColor(deepPink);
            c.beginPath();
            c.rect(directionsPos.x, directionsPos.y, r.width, r.height);
            c.fillPath();
            c.fillColor(white);
            c.text("リピートする　クリックまたはR(r)でリピートをオフ", directionsPos.x, directionsPos.y);
        } else {
            ofRectangle r = c.textBounds("リピートしない　クリックまたはR(r)でリピートをオン", directionsPos.x, directionsPos.y);
            c.fillColor(deepPink);
            c.beginPath();
            c.rect(directionsPos.x, directionsPos.y, r.width, r.height);
            c.fillPath();
            c.fillColor(white);
            c.text("リピートしない　クリックまたはR(r)でリピートをオン", directionsPos.x, directionsPos.y);
        }
    }
    
    //前に戻る、先に進むアイコンの上にマウスがきたらテキストを表示
    if (ofGetWidth()/2-(speedR.x-ofGetWidth()/2+45)/3-45 <= mouseX && mouseX <= ofGetWidth()/2-(speedR.x-ofGetWidth()/2+45)/3+45 && text2Y-15 <= mouseY && mouseY <= text2Y+75) {
        ofRectangle r = c.textBounds("動画の最初または前のブックマークに戻ります(←)", directionsPos.x, directionsPos.y);
        c.fillColor(deepPink);
        c.beginPath();
        c.rect(directionsPos.x, directionsPos.y, r.width, r.height);
        c.fillPath();
        c.fillColor(white);
        c.text("動画の最初または前のブックマークに戻ります(←)", directionsPos.x, directionsPos.y);
    }
    if (ofGetWidth()/2+(speedR.x-ofGetWidth()/2+45)/3-45 <= mouseX && mouseX <= ofGetWidth()/2+(speedR.x-ofGetWidth()/2+45)/3+45 && text2Y-15 <= mouseY && mouseY <= text2Y+75) {
        ofRectangle r = c.textBounds("動画の最後または次のブックマークに進みます(→)", directionsPos.x, directionsPos.y);
        c.fillColor(deepPink);
        c.beginPath();
        c.rect(directionsPos.x, directionsPos.y, r.width, r.height);
        c.fillPath();
        c.fillColor(white);
        c.text("動画の最後または次のブックマークに進みます(→)", directionsPos.x, directionsPos.y);
    }
    
    //ミュージックアイコンの上にマウスがきたらテキストを表示 & ボックスを表示
    if (textPos.x <= mouseX && mouseX <= textPos.x+90 && text2Y-15 <= mouseY && mouseY <= text2Y+75) {
        ofRectangle r = c.textBounds("動画を変える", directionsPos.x, directionsPos.y);
        c.fillColor(deepPink);
        c.beginPath();
        c.rect(directionsPos.x, directionsPos.y, r.width, r.height);
        c.fillPath();
        c.fillColor(white);
        c.text("動画を変える", directionsPos.x, directionsPos.y);
        bVideoBox = true;
    }
    
    if (bVideoBox) {
        for (int i = 0; i < videos.size(); i++) {
            //星の上にマウスオーバーかしてるかどうかのチェック
            if (videoBoxR.x+padding<=mouseX && mouseX <= videoBoxR.x+80 && videoBoxR.y+padding*1.5+videoNameR[i].y<=mouseY && mouseY <= videoBoxR.y+padding*1.5+videoNameTextSize*1.4+videoNameR[i].y) {
                bStarMouseOver[i]=true;
                
                //星の上にマウスがきたらテキストを表示
                ofRectangle r = c.textBounds("星をつけた動画がプログラム起動時に再生されます", directionsPos.x, directionsPos.y);
                c.fillColor(deepPink);
                c.beginPath();
                c.rect(directionsPos.x, directionsPos.y, r.width, r.height);
                c.fillPath();
                c.fillColor(white);
                c.text("星をつけた動画がプログラム起動時に再生されます", directionsPos.x, directionsPos.y);
                
            } else {
                bStarMouseOver[i]=false;
            }
            
        }
        
    }
    
    //----------------------------------------------------説明テキストおわり
    
    //動画タイトルの入ったボックス（テキストと星はcanvas.draw(0,0)の下
    if(bVideoBox){
        c.fillColor(pink); //box(fill)
        c.beginPath();
        c.roundedRect(videoBoxR.x, videoBoxR.y, videoBoxR.width, videoBoxR.height, 10);
        c.fillPath();
        c.lineWidth(5); //box(stroke)
        c.strokeColor(white);
        c.beginPath();
        c.roundedRect(videoBoxR.x, videoBoxR.y, videoBoxR.width, videoBoxR.height, 10);
        c.strokePath();
    }
    
    c.end();
    
    canvas.draw(0, 0); //ofxNanoVGのキャンバスを描画
    
    //星アイコンと動画名
    drawText();
    
    //一時停止アイコン、再生アイコン
    ofPushMatrix();
    ofTranslate(ofGetWidth()/2, text2Y+30);
    ofScale(1/svg.getWidth()*90, 1/svg.getHeight()*90);
    ofTranslate(-svg.getWidth()/2, -svg.getHeight()/2);
    if(bPause){
        icons[0].draw();
    } else {
        icons[1].draw();
    }
    ofPopMatrix();
    
    //ブックマーク
    if (bookmarks.size()>2) {
        for (int i = 1; i < bookmarks.size()-1; i++) {
            ofPushMatrix();
            ofTranslate(bookmarks[i].x, barPos.y-30);
            ofScale(1/svg.getWidth()*50, 1/svg.getHeight()*50);
            ofTranslate(-svg.getWidth()/2, -svg.getHeight()/2);
            if(bookmarks[i].y==0){
                icons[2].setColor(ofColor(255)); //アイコンの色
            } else if(bookmarks[i].y==1){
                icons[2].setColor(ofColor(106, 196, 218)); //マウスオーバー時の色
            } else if(bookmarks[i].y==2){
                icons[2].setColor(ofColor(0, 140, 255)); //選択時の色
                
            }
            if (!(bookmarks[i].x < videoBoxR.x+videoBoxR.width && bVideoBox == true)) {
                //動画タイトルの入ったボックスはこのアイコンより前に描いてるので、ボックスを開いてるときはボックスの上に重ならないように非表示にする
                icons[2].draw();
            }
            
            ofPopMatrix();
        }
    }
    
    //前に戻るアイコン、先に進むアイコン
    ofPushMatrix();
    ofTranslate(ofGetWidth()/2-(speedR.x-ofGetWidth()/2+45)/3, text2Y+30);
    ofScale(1/svg.getWidth()*90, 1/svg.getHeight()*90);
    ofTranslate(-svg.getWidth()/2, -svg.getHeight()/2);
    icons[3].draw();
    ofPopMatrix();
    
    ofPushMatrix();
    ofTranslate(ofGetWidth()/2+(speedR.x-ofGetWidth()/2+45)/3, text2Y+30);
    ofScale(1/svg.getWidth()*90, 1/svg.getHeight()*90);
    ofTranslate(-svg.getWidth()/2, -svg.getHeight()/2);
    icons[4].draw();
    ofPopMatrix();
    
    //リピートアイコン
    ofPushMatrix();
    ofTranslate(textPos.x+45+120, text2Y+30);
    ofScale(1/svg.getWidth()*90, 1/svg.getHeight()*90);
    ofTranslate(-svg.getWidth()/2, -svg.getHeight()/2);
    if(bRepeat){
            icons[5].setColor(ofColor(255));
    } else {
        icons[5].setColor(ofColor(255, 150));
    }
    icons[5].draw();
    ofPopMatrix();
    
    //ミュージックアイコン
    ofPushMatrix();
    ofTranslate(textPos.x+45, text2Y+30);
    ofScale(1/svg.getWidth()*90, 1/svg.getHeight()*90);
    ofTranslate(-svg.getWidth()/2, -svg.getHeight()/2);
    icons[6].draw();
    ofPopMatrix();
    
    //現在のフレーム(int)を保存 updateで使用
    pre_currentFrame = currentFrame;
    
}
//--------------------------------------------------------------
void ofApp::drawText(){
    //星と動画名テキストを描画
    
    if (bVideoBox) {
        //星
        ofPushMatrix();
        ofTranslate(videoBoxR.x, videoBoxR.y);
        ofTranslate(padding+(videoNameTextSize*1.4)/2, padding*1.5+videoNameR[0].height*0.5);
        for (int i = 0; i < videos.size(); i++) {
            ofPushMatrix();
            ofTranslate(0, videoNameR[i].y);
            ofScale(1/svg.getWidth()*25*1.4, 1/svg.getHeight()*25*1.4);
            ofTranslate(-svg.getWidth()/2, -svg.getHeight()/2);
            if(i == favorite){ //お気に入り動画の星
                icons[8].setColor(ofColor(254, 0, 142));
                icons[8].draw();
            } else {
                if (bStarMouseOver[i]){ //星にマウスオーバーしたとき
                    icons[7].setColor(ofColor(255, 255, 255));
                } else { //その他
                    icons[7].setColor(ofColor(255, 150));
                }
                icons[7].draw();
            }
            ofPopMatrix();
        }
        ofPopMatrix();
        
        for (int i = 0; i < videos.size(); i++) {
            //動画タイトルのテキストにマウスオーバーしてるかどうかのチェック
            //マウスオーバー:true
            if (videoBoxR.x+70 <= mouseX && mouseX <= videoBoxR.x+videoBoxR.getWidth()-padding && videoBoxR.y+padding*1.5+videoNameR[i].y <= mouseY && mouseY <= videoBoxR.y+padding*1.5+videoNameR[i].y+videoNameR[0].getHeight()) {
                bVideoNameMouseOver[i] = true;
                
                scrollTime[i]-=1; //文字をスクロール
                if ((-1*scrollTime[i] > videoNameR[i].width)) { //スクロールした距離が動画タイトルの文字幅を超えたら文字の位置をずらす
                    scrollTime[i] = videoBoxR.width-padding-70;
                }
                
            } else { //マウスオーバー:false
                bVideoNameMouseOver[i] = false;
                scrollTime[i] = 0;
            }
        }
        
        //動画タイトル
        ofPushMatrix();
        ofTranslate(videoBoxR.x, videoBoxR.y);
        ofTranslate(70, padding*1.5+videoNameR[0].getHeight());
        for (int i = 0; i < videos.size(); i++) {
            if (videoNameR[i].width > videoBoxR.width-80) { //文字がボックスの幅より長いとき、画面からはみ出る部分を省く
                ofFbo fbo;
                fbo.allocate(videoBoxR.width-padding-70, 25*1.6, GL_RGB32F_ARB);
                fbo.begin();
                ofSetColor(238, 126, 177);
                ofDrawRectangle(0, 0, videoBoxR.width-padding-70, 25*1.6);
                if (i == currentVideo) { //現在再生してる動画
                    ofSetColor(106, 196, 218);
                } else {
                    if(bVideoNameMouseOver[i]){ //マウスオーバー
                        ofSetColor(255, 255, 255);
                    } else { //その他
                        ofSetColor(255, 150);
                    }
                }
                font.drawString(videoName[i], scrollTime[i], videoNameR[0].getHeight());
                fbo.end();
                ofSetColor(255);
                fbo.draw(0, videoNameR[i].y-videoNameR[0].getHeight());
                
            } else { //文字がボックスの幅より短いとき
                if (i == currentVideo) { //現在再生してる動画
                    ofSetColor(106, 196, 218);
                } else {
                    if(bVideoNameMouseOver[i]){ //マウスオーバー
                        ofSetColor(255, 255, 255);
                    } else { //その他
                        ofSetColor(255, 150);
                    }
                }
                font.drawString(videoName[i], 0, videoNameR[i].y);
            }
        }
        ofPopMatrix();
    }
}
//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    // s : スクリーンショット
    // shift : ブックマークを追加
    // delete, backspace : //選択したブックマークを削除
    // ← : 前のブックマークに戻る
    // → : 次のブックマークに進む
    // r : リピート←→ノーリピート
    // space : 再生←→一時停止
    // ↑ : 再生スピードをあげる
    // ↓ : 再生スピードをさげる
    if (key == 'S' || key == 's') { //スクリーンショット
        ofImage myImage;
        myImage.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
        myImage.save(ofGetTimestampString("%Y%m%d%H%M%S")+"##.png");
    }
    if (key == OF_KEY_SHIFT){ //ブックマークを追加
        //配列にすでに入ってる数値と同じ数値がないかチェックする
        //同じ数値の場合は追加しない
        int c = 0;
        for (int i = 0; i < bookmarks.size(); i++) {
            if (bookmarks[i].x == currentPosX) c = 1;
            if (c == 1) break;
        }
        if (c == 0) {
            bookmarks.pop_back(); //最終要素を削除
            bookmarks.push_back(glm::vec2(currentPosX, 0)); //現在の再生位置をブックマークに追加
            bookmarks.push_back(glm::vec2(ofGetWidth()-barPos.x, 0)); //シークバーのラスト位置を追加
            
            //xml
            string str = removeSpaces(videoName[currentVideo]);
            drawing.removeChild(str);
            auto bm = drawing.appendChild(str);
            for (int i = 0; i < bookmarks.size(); i++) {
                bm.appendChild("pt").set(bookmarks[i].x);
            }
            
        }
    }

    if (key == OF_KEY_DEL || key == OF_KEY_BACKSPACE) { //選択したブックマークを削除
        if(bookmarks.size()>2){
            for (int i = 1; i < bookmarks.size()-1; i++) {
                if (bookmarks[i].y == 2) {
                    bookmarks.erase(bookmarks.begin()+i);
                    
                    //xml
                    string str = removeSpaces(videoName[currentVideo]);
                    drawing.removeChild(str);
                    auto bm = drawing.appendChild(str);
                    for (int i = 0; i < bookmarks.size(); i++) {
                        bm.appendChild("pt").set(bookmarks[i].x);
                    }
                }
            }
        }
    }

    if (key == OF_KEY_LEFT){ //前のブックマークに戻る
        backBookmark();
    }
    if (key == OF_KEY_RIGHT){ //次のブックマークに進む
        nextBookmark();
    }
    if (key =='R' || key =='r'){ //リピート←→ノーリピート
        settingRepeat();
    }
    if (key == ' '){ //再生←→一時停止
        settingPlay();
    }
    if (key == OF_KEY_UP && speed < 1.0){ //再生スピードをあげる
        speed += 0.1;
        speed = round(speed*10)/10; //小数点以下第２位で四捨五入
        videos[currentVideo].setSpeed(speed);
        
    }
    if (key == OF_KEY_DOWN && speed-0.1 >= 0.3){ //再生スピードをさげる
        speed -= 0.1;
        speed = round(speed*10)/10; //小数点以下第２位で四捨五入
        videos[currentVideo].setSpeed(speed);
    }
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

    //再生スピードのボックスを消す（マウスがボックスと再生スピードテキスト以外の場所に行ったら）
    if (!(speedBoxR.x<=mouseX && mouseX<=speedBoxR.x+speedBoxR.width && speedBoxR.y<=mouseY && mouseY<=speedBoxR.y+speedBoxR.height)){
        if (!(speedR.x<=mouseX && mouseX<=speedR.x+speedR.width
            && speedR.y<=mouseY && mouseY <= speedR.y+speedR.height)){
            bSpeedBar = false;
        }
    }
    
    //動画タイトルの入ったボックスを消す（マウスがボックスとミュージックアイコン以外の場所に行ったら）
    if (!(videoBoxR.x <= mouseX && mouseX <= videoBoxR.x+videoBoxR.width && videoBoxR.y <= mouseY && mouseY <= videoBoxR.y+videoBoxR.height)) {
        if (!(textPos.x <= mouseX && mouseX <= textPos.x+90 && text2Y-15 <= mouseY && mouseY <= text2Y+75)){
            bVideoBox = false;
        }
    }
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    if(!bVideoBox){
        //シークバーの丸の位置を変える
        if(barPos.x-5 <= x && x <= ofGetWidth()-barPos.x+5 && barPos.y-9 <= y && y <= barPos.y+9){
            changeSeekEllipse(x);
        }
        //再生スピードバーの丸の位置を変える
        if (sBarR.x-7.5 <= x && x <= sBarR.x+7.5 && sBarR.y-5 <= y && y <= sBarR.y+sBarR.height+5) {
            changeSpeedEllipse(y);
        }
    }
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    if(!bVideoBox){
        //シークバーの丸の位置を変える
        if(barPos.x <= x-5 && x <= ofGetWidth()-barPos.x+5 && barPos.y-9 <= y && y <= barPos.y+9){
            changeSeekEllipse(x);
        }
        
        //再生スピードバーの丸の位置を変える
        if (sBarR.x-7.5 <= x && x <= sBarR.x+7.5 && sBarR.y-5 <= y && y <= sBarR.y+sBarR.height+5) {
            changeSpeedEllipse(y);
        }
        
        //ブックマークを選択
        if(bookmarks.size()>2){
            for (int i = 1; i < bookmarks.size()-1; i++) {
                if (bookmarks[i].x-25 <= mouseX && mouseX <= bookmarks[i].x+25 && barPos.y-55 <= mouseY && mouseY <= barPos.y-5) {
                    bookmarks[i].y = 2;
                } else {
                    bookmarks[i].y = 0;
                }
            }
        }
    }
    
    //リピートアイコンを押したとき
    if (textPos.x+120<=mouseX && mouseX <= textPos.x+120+90 && text2Y-15 <= mouseY && mouseY <= text2Y+75) {
        settingRepeat();
    }
    
    //前に戻るアイコンを押したとき
    if (ofGetWidth()/2-(speedR.x-ofGetWidth()/2+45)/3-45 <= mouseX && mouseX <= ofGetWidth()/2-(speedR.x-ofGetWidth()/2+45)/3+45 && text2Y-15 <= mouseY && mouseY <= text2Y+75){
        backBookmark();
    }
    
    //再生、一時停止（アイコンを押したとき）
    if (ofGetWidth()/2-45 <= x && x <= ofGetWidth()/2+45 && text2Y+30-45 <= y && y <= text2Y+30+45) {
        settingPlay();
    }
    
    //先に進むアイコンを押したとき
    if (ofGetWidth()/2+(speedR.x-ofGetWidth()/2+45)/3-45 <= mouseX && mouseX <= ofGetWidth()/2+(speedR.x-ofGetWidth()/2+45)/3+45 && text2Y-15 <= mouseY && mouseY <= text2Y+75) {
        nextBookmark();
    }
    
    if(bVideoBox){
        for (int i = 0; i < videos.size(); i++) {
            //動画タイトル名の入ったボックス内の星を押したとき
            if (videoBoxR.x+padding<=mouseX && mouseX <= videoBoxR.x+padding+videoNameTextSize*1.4 && videoBoxR.y+padding*1.5+videoNameR[i].y<=mouseY && mouseY <= videoBoxR.y+padding*1.5+videoNameTextSize*1.4+videoNameR[i].y) {
                favorite = i;
                //xmlに追加
                drawing.removeChild("fav");
                drawing.appendChild("fav").set(i);
            }
            
            //動画タイトルのテキストを押したとき
            if (videoBoxR.x+80 <= mouseX && mouseX <= videoBoxR.x+videoBoxR.getWidth()-padding && videoBoxR.y+padding+videoNameR[i].y <= mouseY && mouseY <= videoBoxR.y+padding*1.5+videoNameR[i].y+videoNameR[0].getHeight()) {
                if(i != currentVideo){ //押した動画がいま再生してる動画でないとき、新しい動画に切り替える
                    videos[currentVideo].stop();
                    currentVideo = i;
                    videos[currentVideo].play();
                    
                    //xmlからブックマークをロードする
                    bookmarks.clear();
                    bookmarks.resize(2);
                    string str = removeSpaces(videoName[currentVideo]); //動画名にスペースや！が入ってる場合、削除する
                    string str2 = "//"+str;
                    auto posXml = xml.findFirst(str2); //動画名と一致するタグがxml内にあるか検索
                    if (!posXml) { //↑なければ新しく追加
                        drawing.removeChild(str);
                        auto bm = drawing.appendChild(str);
                        bm.appendChild("pt").set(barPos.x);
                        bm.appendChild("pt").set(ofGetWidth()-barPos.x);
                        
                        bookmarks[0] = glm::vec2(barPos.x, 0); //y 0:通常表示 1:マウスオーバー 2:クリックして選択中
                        bookmarks[1] = glm::vec2(ofGetWidth()-barPos.x, 0);
                    } else {
                        //xmlにデータがある場合はこっちから取得
                        auto pts = posXml.getChildren("pt");
                        int j = 0;
                        for (auto & pt: pts) {
                            auto x = pt.getFloatValue();
                            if(j < 2){ //配列を最初に２個分だけ確保してるので、３つ以上データがある場合はpush_backで追加する
                                bookmarks[j] = glm::vec2(x, 0);
                            } else {
                                bookmarks.push_back(glm::vec2(x, 0));
                            }
                            j++;
                        }
                    }
                    
                    //------------------------------------xml
                }
            }
        }
    }
}
//--------------------------------------------------------------
void ofApp::changeSeekEllipse(int x){ //シークバーの丸の位置を変える
    currentPosX = x;
    int frame = round(ofMap(currentPosX, barPos.x, ofGetWidth()-barPos.x, 0, totalFrame[currentVideo])); //round:小数点以下第１位で四捨五入
    videos[currentVideo].setFrame(frame);
    
}
//--------------------------------------------------------------
void ofApp::changeSpeedEllipse(int y){ //再生スピードバーの丸の位置を変える
    float sp = ofMap(y, sBarR.y+sBarR.height+5, sBarR.y-5, 0.3, 1.0);
    speed = sp;
    sp = round(sp*10)/10; //小数点以下第２位で四捨五入
    videos[currentVideo].setSpeed(sp);
}
//--------------------------------------------------------------
void ofApp::backBookmark(){ //前のブックマークに戻る
    int target;
    float rngMin;
    //一番近い目標を探す
    for (int num = 0; num < bookmarks.size(); num++) {
        if (bookmarks[num].x < currentPosX-3) {
            //-3:前のブックマークに戻ったあとにすぐ←を押したら、またさらに前のブックマークに戻れるようにする
            float rng = currentPosX-bookmarks[num].x;
            
            //距離を比べる
            //numが０のときはまず0を一番近いターゲットとする
            if (rng < rngMin || num == 0) {
                target = num; //目標の番号格納
                rngMin = rng; //距離入れ替え
            }
        }
    }
    currentPosX = bookmarks[target].x;
    int frame = round(ofMap(currentPosX, barPos.x, ofGetWidth()-barPos.x, 0, totalFrame[currentVideo])); //round:小数点以下第１位で四捨五入
    videos[currentVideo].setFrame(frame);
}
//--------------------------------------------------------------
void ofApp::nextBookmark(){ //次のブックマークに進む
    int target;
    float rngMin;
    //一番近い目標を探す
    //配列の最後からチェックする
    for (int num = bookmarks.size()-1; num >= 0; num--) {
        if (currentPosX < bookmarks[num].x) {
            float rng = bookmarks[num].x - currentPosX;
            
            //距離を比べる
            //ひとつめはまず一番近いターゲットとする
            if (rng < rngMin || num == bookmarks.size()-1) {
                target = num; //目標の番号格納
                rngMin = rng; //距離入れ替え
            }
        }
    }
    currentPosX = bookmarks[target].x;
    int frame = round(ofMap(currentPosX, barPos.x, ofGetWidth()-barPos.x, 0, totalFrame[currentVideo])); //round:小数点以下第１位で四捨五入
    videos[currentVideo].setFrame(frame);
}
//--------------------------------------------------------------
void ofApp::settingRepeat(){ //リピート←→ノーリピート
    bRepeat = !bRepeat;
    if (bRepeat) {
        videos[currentVideo].setLoopState(OF_LOOP_NORMAL); //リピートする
    } else {
        videos[currentVideo].setLoopState(OF_LOOP_NONE); //リピートしない
    }
    
}
//--------------------------------------------------------------
void ofApp::settingPlay(){ //再生←→一時停止
    if(currentFrame == totalFrame[0]-1 && bRepeat == false){ //フレームが最後の時　＆　ノーリピートの場合
        //ビデオを最初に戻して再生
        count = 0;
    }
    bPause = !bPause;
    videos[currentVideo].setPaused(bPause);
}
//--------------------------------------------------------------
void ofApp::exit(){
    //プログラムを終了したときにxmlを保存
    xml.save("parameter.xml");
}
//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
