#include <cstdio>
#include <iostream>
#include <cstring>
#include <cmath>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <string>
#include <ale_interface.hpp>
#include <TPG.h>
#include "ALEScreenEx.cc"
#include <bitset>
#define PIXEL_SKIP 2
#define COLOR_FACTOR 10
using namespace std;

class FeatureMap{
   private:
      bool _backgroundDetection = true;
      int _blockHeight;
      int _blockWidth;
      bitset<8> _colourBits;
      ColourPalette _palette_NTSC;
      ColourPalette _palette_SECAM;
      ALEScreenEx _background = ALEScreenEx(ALE_SCREEN_HEIGHT,ALE_SCREEN_WIDTH);
      ALEScreenEx _foreground = ALEScreenEx(ALE_SCREEN_HEIGHT,ALE_SCREEN_WIDTH);
      ALEScreenEx _featureFrame = ALEScreenEx(ALE_SCREEN_HEIGHT,ALE_SCREEN_WIDTH);
      //ALEScreenEx _gridFrame = ALEScreenEx(ALE_SCREEN_HEIGHT,ALE_SCREEN_WIDTH);
      ALEScreenEx _screen = ALEScreenEx(ALE_SCREEN_HEIGHT,ALE_SCREEN_WIDTH);
      ALEScreenEx _visualField = ALEScreenEx(ALE_SCREEN_HEIGHT,ALE_SCREEN_WIDTH);
      int _numColours = 8;//SECAM
      int _numColumns = 32;//16
      int _numFeatures;
      int _numRows = 42;//21
      bool _SECAM = false;
      bool _saveFrames = false;
      bool _quantize = true;
      vector  <int> _colourTally;

   public:
      FeatureMap(string bg, bool quantize, bool SECAM, bool backgroundDetection){
         if (backgroundDetection) _background.load(bg);
         _backgroundDetection = backgroundDetection;
         _quantize = quantize;
         _SECAM = SECAM;
         _colourTally.reserve(255);
         _colourTally.resize(255);
         reset(_colourTally);
         _palette_NTSC.setPalette("standard","NTSC");
         _palette_SECAM.setPalette("standard","SECAM");
         _blockHeight = ALE_SCREEN_HEIGHT / _numRows;
         _blockWidth = ALE_SCREEN_WIDTH / _numColumns;
         if (_quantize) _numFeatures = _numRows * _numColumns;
         else _numFeatures = 210 * 160; //8400
      };

      inline int encodeSECAM(int color) { return (color & 0xF) >> 1; }

      inline void getFeatures(const ALEScreen &aleScreen,vector<double> &currentState, set<long> &indexedFeatures, const char* suffix) {
         if (_saveFrames){
            _foreground.reset(aleScreen.width(), aleScreen.height());
            _featureFrame.reset(aleScreen.width(), aleScreen.height());
            _visualField.reset(aleScreen.width(), aleScreen.height());
            _screen.copy(aleScreen);
         }

         if (_quantize){
            int featuresPerBlock = _numColours;
            int blockIndex = 0;
            // For each pixel block
            int pixelOffset;//Offset pixel samples like a checker board
            for (int by = 0; by < _numRows; by++) {
               for (int bx = 0; bx < _numColumns; bx++) {
                  _colourBits.reset();
                  int xo = bx * _blockWidth;
                  int yo = by * _blockHeight;
                  // Only process this block if the agent actually looks at it or if we're saving frames
                  bool activeBlock = indexedFeatures.find(_numColumns*by+bx) != indexedFeatures.end();
                  if (activeBlock || _saveFrames){
                     // Determine which colors are present; ignore background
                     pixelOffset = (by%2==0 && bx%2==0) || (by%2!=0 && bx%2!=0) ? 0 : 1;
                     for (int x = xo; x < xo + _blockWidth; x++, pixelOffset++)
                        for (int y = (pixelOffset%2==0 ? yo : yo+1); y < yo + _blockHeight; y += PIXEL_SKIP) {
                           unsigned char pixel = aleScreen.get(y,x);
                           if (!_backgroundDetection || (_backgroundDetection && pixel != _background.get(y,x))){ 
                              _colourBits.set(encodeSECAM((int)pixel));
                              if (_saveFrames) _foreground.set(y,x,pixel);
                           }
                        }
                     // Add all colors present to our feature set
                     currentState[_numColumns*by+bx] = _colourBits.to_ulong(); 
                     // Save frames for visual debugging
                     if (_saveFrames){
                        for (int x = xo; x < xo + _blockWidth; x++)
                           for (int y = yo; y < yo + _blockHeight; y++){
                              _featureFrame.set(y,x,(_colourBits.to_ulong() == 1 ? 0 : (_colourBits.to_ulong()*COLOR_FACTOR)%127));//multiply by 10 to better distribute colours
                              _visualField.set(y,x,activeBlock ? _screen.get(y,x) : 77);  //show the screen pixels or grey
                              //_gridFrame.set(y,x,(yo*xo)%128);
                           }
                     }
                  }
                  blockIndex += featuresPerBlock;
               }
            }
         }

         //else{//this version drops pixels in checker board pattern, leaving 8400 features
         //   int i = 0;
         //   int rowCount = 0;
         //   if(_saveFrames){
         //      for (int r = 0; r < aleScreen.height(); r+=2, rowCount++)
         //         for (int c = rowCount%2==0?0:1; c < aleScreen.width(); c+=2){
         //            currentState[i] = encodeSECAM(aleScreen.get(r,c));
         //            _featureFrame.set(r,c,aleScreen.get(r,c)); //SECAM encoded by ColorPalette when saved
         //            _visualField.set(r,c,indexedFeatures.find(i) != indexedFeatures.end() ? aleScreen.get(r,c) : 77); //show the screen pixels or grey
         //            i++;
         //         }
         //   }
         //   else{
         //      for (int r = 0; r < aleScreen.height(); r+=2, rowCount++)
         //         for (int c = rowCount%2==0?0:1; c < aleScreen.width(); c+=2)
         //            currentState[i++] = encodeSECAM(aleScreen.get(r,c));
         //   }

         else{//this version inlcudes all pixels, 33600 features
            int i = 0;
            if(_saveFrames){
               for (int r = 0; r < aleScreen.height(); r++)
                  for (int c = 0; c < aleScreen.width(); c++){
                     if (_SECAM)
                        currentState[i] = encodeSECAM(aleScreen.get(r,c));
                     else
                        currentState[i] = aleScreen.get(r,c) >> 1;
                     _featureFrame.set(r,c,aleScreen.get(r,c)); //possibly SECAM encoded by ColorPalette when saved
                     _visualField.set(r,c,indexedFeatures.find(i) != indexedFeatures.end() ? aleScreen.get(r,c) : 77); //show the screen pixels or grey
                     i++;
                  }
            }
            else{
               for (int r = 0; r < aleScreen.height(); r++)
                  for (int c = 0; c < aleScreen.width(); c++)
                     if (_SECAM)
                        currentState[i++] = encodeSECAM(aleScreen.get(r,c));
                     else
                        currentState[i++] = aleScreen.get(r,c) >> 1;
            }
         }
         if (_saveFrames){
            ScreenExporter seNTSC(_palette_NTSC);
            ScreenExporter seSECAM(_palette_SECAM);
            char frameName[80];
            sprintf(frameName, "%s%s%s","screenFrame.",suffix,".png"); seNTSC.save(_screen,frameName);
            sprintf(frameName, "%s%s%s","featureFrame.",suffix,".png"); 
            if (_quantize) 
               seNTSC.save(_featureFrame,frameName); 
            else {
               if (_SECAM) seSECAM.save(_featureFrame,frameName);
               else seNTSC.save(_featureFrame,frameName);
            }
            sprintf(frameName, "%s%s%s","visualFrame.",suffix,".png"); seNTSC.save(_visualField,frameName);
            //sprintf(frameName, "%s%05d%s","gridFrame.",suffix,".png"); seNTSC.save(_gridFrame,frameName);
         }
      }

      inline int indexOfMaxElement(vector <int> &v){
         int maxIndex = 0;
         int maxInt = 0;
         for (size_t i = 0; i < v.size(); i++) 
            if (v[i] > maxInt) {
               maxInt = v[i];
               maxIndex = i;
            }
         return maxIndex;
      }

      inline int maxFeatureVal() { return _quantize ? 255 : (_SECAM ? 8 : 128); }

      inline int minFeatureVal() { return 0; }

      inline int numFeatures() {
         return _numFeatures;
      }

      inline void reset(vector<int> &v) { for (size_t i = 0; i < v.size(); i++) v[i] = 0; }
      inline void saveFrames(bool sf) { _saveFrames = sf; }
      };
