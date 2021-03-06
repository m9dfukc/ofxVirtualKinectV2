#include "ofxVirtualKinectV2.h"

const unsigned int camWidth = 512;
const unsigned int camHeight = 424;

ofxVirtualKinectV2::ofxVirtualKinectV2() :
newFrame(false),
maxLength(100),
stepSize(2),
nearClipping(0),
farClipping(1024),
orthoScale(10),
position(ofVec3f(0, 0, 0)),
sceneRotation(ofVec3f(0, 0, 0)),
cameraRotation(ofVec3f(0, 0, 0)),
horizontalFlip(true) {
}

void ofxVirtualKinectV2::setup() {
	surface.resize(camWidth * camHeight);
	indices.resize(camWidth * camHeight * 3);

	kinect.open(false, true);
    kinect.start();

	fbo.allocate(camWidth, camHeight, GL_RGB);
	colorImage.allocate(camWidth, camHeight, OF_IMAGE_COLOR);
	grayImage.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
}

void ofxVirtualKinectV2::close() {
    kinect.close();
}

void ofxVirtualKinectV2::updateSurface() {
    for(int y = 0; y < camHeight; y += stepSize) {
        for(int x = 0; x < camWidth; x += stepSize) {
            int i = y * camWidth + x;
            surface[i] = kinect.getWorldCoordinateAt(x, y);
        }
    }
}

void ofxVirtualKinectV2::updateMesh() {
    float* z = kinect.getDepthPixelsRef().getData();
    if (z == NULL) return;
    indices.clear();
    for(int y = 0; y < camHeight - stepSize; y += stepSize) {
        for(int x = 0; x < camWidth - stepSize; x += stepSize) {
            int i = y * camWidth + x;
            unsigned int nwi = i;
            unsigned int nei = nwi + stepSize;
            unsigned int swi = nwi + (stepSize * camWidth);
            unsigned int sei = swi + stepSize;
            float nw = z[nwi];
            float ne = z[nei];
            float sw = z[swi];
            float se = z[sei];

            if(nw != 0 && ne != 0 && sw != 0 &&
                 abs(nw - ne) < maxLength && abs(nw - sw) < maxLength) {
                indices.push_back(nwi);
                indices.push_back(nei);
                indices.push_back(swi);
            }

            if(ne != 0 && se != 0 && sw != 0 &&
                 abs(sw - se) < maxLength && abs(ne - se) < maxLength) {
                indices.push_back(nei);
                indices.push_back(sei);
                indices.push_back(swi);
            }
        }
    }
}

void ofxVirtualKinectV2::renderCamera() {
	fbo.begin();
	ofClear(0, 255);
    
	glEnable(GL_FOG);

	glClearColor(0, 0, 0, 1);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	GLfloat fogColor[4]= {0, 0, 0, 1};
	glFogfv(GL_FOG_COLOR, fogColor);
	glHint(GL_FOG_HINT, GL_FASTEST);
	glFogf(GL_FOG_START, nearClipping);
	glFogf(GL_FOG_END, farClipping);

	ofPushMatrix();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-256 * orthoScale, 256 * orthoScale, -212 * orthoScale, 212 * orthoScale, nearClipping, farClipping);
	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();

	ofRotateX(sceneRotation.x);
	ofRotateY(sceneRotation.y);
	ofRotateZ(sceneRotation.z - 180);
	ofTranslate(position.x, position.y, position.z);
	ofRotateX(cameraRotation.x);
	ofRotateY(cameraRotation.y);
	ofRotateZ(cameraRotation.z);

	ofScale(horizontalFlip ? -1 : 1, 1, -1);

    ofPushStyle();
    ofEnableDepthTest();
	ofSetColor(255);
    ofFill();
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), &(surface[0].x));
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, &indices[0]);
	glDisableClientState(GL_VERTEX_ARRAY);
    ofPopStyle();

	ofPopMatrix();

	glDisable(GL_FOG);
    
    ofDisableDepthTest();

	fbo.end();
}

void ofxVirtualKinectV2::updatePixels() {
	fbo.readToPixels(colorImage);
    unsigned char* grayPixels = grayImage.getData();
	unsigned char* colorPixels = colorImage.getData();
	int n = camWidth * camHeight;
	for(int i = 0; i < n; i++) {
		*grayPixels = *colorPixels;
		grayPixels += 1;
		colorPixels += 3;
	}
}

void ofxVirtualKinectV2::update() {
    kinect.update();
    if(kinect.isFrameNew()) {
        newFrame = true;
        updateSurface();
        updateMesh();
        updatePixels();
        renderCamera();
    }
}

bool ofxVirtualKinectV2::isFrameNew() {
	bool curNewFrame = newFrame;
	newFrame = false;
	return curNewFrame;
}

ofPixels& ofxVirtualKinectV2::getPixels() {
	return grayImage;
}

const ofPixels& ofxVirtualKinectV2::getPixels() const {
    return grayImage;
}


void ofxVirtualKinectV2::draw(float x, float y) {
	fbo.draw(x, y);
}

void ofxVirtualKinectV2::setMaxLength(float maxLength) {
	this->maxLength = maxLength;
}

void ofxVirtualKinectV2::setStepSize(int stepSize) {
	this->stepSize = stepSize;
}

void ofxVirtualKinectV2::setClipping(float nearClipping, float farClipping) {
	this->nearClipping = nearClipping;
	this->farClipping = farClipping;
}

void ofxVirtualKinectV2::setOrthoScale(float orthoScale) {
	this->orthoScale = orthoScale;
}

void ofxVirtualKinectV2::setPosition(ofVec3f position) {
	this->position = position;
}

void ofxVirtualKinectV2::setCameraRotation(ofVec3f cameraRotation) {
	this->cameraRotation = cameraRotation;
}

void ofxVirtualKinectV2::setSceneRotation(ofVec3f sceneRotation) {
	this->sceneRotation = sceneRotation;
}

void ofxVirtualKinectV2::setHorizontalFlip(bool horizontalFlip) {
	this->horizontalFlip = horizontalFlip;
}

float ofxVirtualKinectV2::grayToDistance(unsigned char value) const {
    return (1 - (float(value) / 255.)) * (farClipping - nearClipping) + nearClipping;
}

int ofxVirtualKinectV2::getWidth() const {
	return camWidth;
}

int ofxVirtualKinectV2::getHeight() const {
	return camHeight;
}
