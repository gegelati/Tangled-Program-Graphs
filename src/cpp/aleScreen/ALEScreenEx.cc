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
#define ALE_SCREEN_WIDTH 160
#define ALE_SCREEN_HEIGHT 210

using namespace std;

class ALEScreenEx : public ALEScreen {
   public:
      ALEScreenEx(int h, int w) : ALEScreen(h,w) {
      }

      void load(string filename){
         ifstream ifs;
         ifs.open(filename.c_str(), ios::in);
         if (!ifs.is_open() || ifs.fail()) die(__FILE__, __FUNCTION__, __LINE__, "Can't open file.");
         string oneline;
         char delim=',';
         vector < string > outcomeFields;
         int y = -1;
         while (getline(ifs, oneline)){
            outcomeFields.clear();
            split(oneline,delim,outcomeFields);
            if (y == -1) {
               m_columns = atoi(outcomeFields[0].c_str());
               m_rows = atoi(outcomeFields[1].c_str());
            }
            else{
               if (outcomeFields.size() != m_columns) die(__FILE__, __FUNCTION__, __LINE__, "Bad screen file.");
               for (int x = 0; x < outcomeFields.size(); x++)
                  set(y,x,atoi(outcomeFields[x].c_str()));
            }
            y++;
         }
         ifs.close();
      }

      inline void reset(int width, int height){
         m_columns = width;
         m_rows = height;
         for(int y = 0; y < m_rows; y++)
            for(int x = 0; x < m_columns; x++)
               set(y,x,0);
      }

      void save(string filename){
         ofstream ofs;
         ofs.open(filename, ios::out);
         if (!ofs.is_open() || ofs.fail()) die(__FILE__, __FUNCTION__, __LINE__, "Can't open file.");
         ofs << m_columns << "," << m_rows << endl;
         for (int y = 0; y < m_rows; y++){
            for (int x = 0; x < m_columns-1; x++)
               ofs << (int)get(y,x) << ",";
            ofs << (int)get(y,m_columns-1) << endl;
         }
         ofs.close();
      }

      inline void set(int r, int c, int val){ 
         // Perform some bounds-checking
         assert (r >= 0 && r < m_rows && c >= 0 && c < m_columns && val <= 255);
         m_pixels[r * m_columns + c] = (pixel_t) val;
      }

      inline void copy(const ALEScreen &aleScreen){
         m_columns = aleScreen.width();
         m_rows = aleScreen.height();
         for(int y = 0; y < m_rows; y++)
            for(int x = 0; x < m_columns; x++)
               set(y,x,aleScreen.get(y,x));
      }
};
