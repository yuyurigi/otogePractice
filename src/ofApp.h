#pragma once

#include "ofMain.h"
#include "ofxNanoVG.h"
#include "ofxSvg.h"
#include "ofxTrueTypeFontUC.h"
#include <regex>


class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
    void setSvg();
    void setVideo();
    string convertVideoTitle(const string &str);
    string removeSpaces(const string& s);
    void drawText();
    void changeSeekEllipse(int x);
    void changeSpeedEllipse(int y);
    void backBookmark();
    void nextBookmark();
    void settingRepeat();
    void settingPlay();
    void exit();
    
    //vector<vector<glm::vec2>> bookmarks;
    vector<glm::vec2> bookmarks;
    int currentVideo, count;
    int currentFrame, pre_currentFrame, textSize;
    float text2Y, speed, currentPosX;
    bool bPause, bSpeedBar, bRepeat;
    glm::vec2 barPos, textPos, directionsPos;
    ofRectangle speedR, speedBoxR, sBarR;
    string text;
    
    //video
    vector<ofVideoPlayer> videos;
    vector<int> totalFrame;
    vector<string> totalTimeText;
    
    //videobox
    vector<string> videoName;
    vector<ofRectangle> videoNameR;
    vector<bool> bStarMouseOver;
    vector<bool> bVideoNameMouseOver;
    vector<int> scrollTime;
    int videoNameTextSize, padding, favorite;
    bool bVideoBox;
    ofRectangle videoBoxR;
    ofxTrueTypeFontUC font;
    
    //ofxNanoVG
    ofxNanoVG::Canvas canvas;
    ofFloatColor pink, deepPink, white, lightBlue, yellow;
    
    //svg
    vector<ofPath> icons;
    ofRectangle svg;
    
    //xml
    ofXml xml;
    ofXml drawing;
};
