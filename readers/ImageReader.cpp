//
// Created by amourao on 26-06-2019.
//

#include "ImageReader.h"
#include "../utils/ImageDecoder.h"

ImageReader::ImageReader(std::string filename) {
  // bundle_fusion_apt0;0;0;30

  // std::string sceneDesc;
  // unsigned int sensorId;
  // unsigned int deviceId;

  std::ifstream file(filename);
  std::string line;
  getline(file, line);
  std::string value;

  std::stringstream ss(line);
  getline(ss, sceneDesc, ';');

  std::string sensorIdStr, deviceIdStr, frameCountStr, fpsStr, frameTypeStr;
  getline(ss, deviceIdStr, ';');
  getline(ss, sensorIdStr, ';');
  getline(ss, frameTypeStr, ';');
  getline(ss, fpsStr);

  sensorId = std::stoul(sensorIdStr);
  deviceId = std::stoul(deviceIdStr);
  frameType = std::stoul(frameTypeStr);
  fps = std::stoul(fpsStr);

  // get frame count
  getline(file, frameCountStr);
  unsigned int frameCount = std::stoul(frameCountStr);

  while (getline(file, line))
    frameLines.push_back(line);

  if (frameCount != frameLines.size())
    std::cerr << "Warning: lines read do not match expected size: "
              << frameLines.size() << " read vs. " << frameCount << " expected."
              << std::endl;

  streamId = randomString(16);

  cps = nullptr;
  reset();
}

std::vector<unsigned char> ImageReader::readFile(std::string &filename) {
  std::streampos fileSize;
  std::ifstream file(filename, std::ios::binary);

  file.seekg(0, std::ios::end);
  fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<unsigned char> fileData(fileSize);
  file.read((char *)&fileData[0], fileSize);
  return fileData;
}

FrameStruct *ImageReader::createFrameStruct(unsigned int frameId) {
  // 0;/home/amourao/data/bundle_fusion/apt0/frame-000000.color.jpg;/home/amourao/data/bundle_fusion/apt0/frame-000000.color.jpg

  std::string line = frameLines[frameId];
  std::stringstream ss(line);

  std::string frameIdStr, framePath;

  getline(ss, frameIdStr, ';');
  getline(ss, framePath);

  unsigned int readFrameId = std::stoul(frameIdStr);

  if (readFrameId != currentFrameCounter)
    std::cerr << "Warning: frame ids do not match: " << readFrameId
              << " read vs. " << currentFrameCounter << " expected."
              << std::endl;

  std::vector<unsigned char> fileData = readFile(framePath);
  FrameStruct *frame = new FrameStruct();

  frame->messageType = 0;

  frame->frameDataType = 0;
  frame->sceneDesc = sceneDesc;
  frame->deviceId = deviceId;
  frame->sensorId = sensorId;
  frame->frameType = frameType;
  frame->timestamps.push_back(1000.0 / fps * currentFrameCounter);
  frame->timestamps.push_back(currentTimeMs());

  frame->frameId = readFrameId;

  frame->frame = fileData;
  frame->streamId = streamId;

  if (cps == nullptr) {
    ImageDecoder id;
    AVFrame *avframe = av_frame_alloc();
    id.imageBufferToAVFrame(frame, avframe);
    cps = new CodecParamsStruct(frame->codec_data);
    av_frame_free(&avframe);
  }
  frame->codec_data = *cps;

  return frame;
}

unsigned int ImageReader::currentFrameId() { return currentFrameCounter; }

std::vector<FrameStruct *> ImageReader::currentFrame() {
  std::vector<FrameStruct *> v;
  v.push_back(currentFrameInternal);
  return v;
}

void ImageReader::nextFrame() {
  currentFrameCounter += 1;
  currentFrameInternal = createFrameStruct(currentFrameCounter);
}

bool ImageReader::hasNextFrame() {
  return currentFrameCounter + 1 < frameLines.size();
}

void ImageReader::goToFrame(unsigned int frameId) {
  currentFrameCounter = frameId;
  currentFrameInternal = createFrameStruct(currentFrameCounter);
}

void ImageReader::reset() {
  currentFrameCounter = 0;
  currentFrameInternal = createFrameStruct(currentFrameCounter);
}

unsigned int ImageReader::getFps() { return fps; }

std::vector<uint> ImageReader::getType() {
  std::vector<uint> result;
  result.push_back(frameType);
  return result;
}