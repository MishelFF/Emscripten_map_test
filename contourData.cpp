#include <iostream>
#include <fstream>

#include "contourData.h"
#include "strFunctions.h"


int ContourData::loadConturs(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return 1; 
    }
    conturs.clear();
    Contour current_path;
    double x, y;
    int n=0;
    std::string line;
    std::vector<int> t;
    while (std::getline(file,line)) if (!line.empty()){
//        std::cerr << line << std::endl;
        size_t pos = 0;size_t start=0;
        pos = line.find_first_of(",;");start=pos+1;
        n=std::stoi(line.substr(0, pos));
        t.clear();
        current_path.clear();
        while (pos< (line.size()-1)) {
            pos = line.find_first_of(",;",start);
            if (pos==std::string::npos) {pos=line.size()-1;}
            std::string subs=trim(line.substr(start, pos-start));
            //std::cerr << subs << std::endl;
            if (is_digits(subs)) {
                t.push_back(std::stoi(subs));
            }
            start=pos+1;
        }
        std::cerr<<"n= " << n <<" type= " << t[0] << std::endl;
        if (!t.empty()){current_path.type=t.front();}
        if(t.size()>1){current_path.hole=t[2];}
        for(int i=0;i<n;i++){
                file >> x >> y;
                current_path.addPoint(PointD{x,y});
//               std::cerr << "x= "<<x<< "  y="<<y<<std::endl;
        }
        file.ignore(); 
        if (!current_path.empty()) {
                conturs.push_back(current_path);
        }
    }
    file.close();
    return 0;  
}
SDL_Color ContourData::getColorByType(const int type){
    if(type==0||type==1) return {0,50,255,255};
    if(type==2||type==3) return {255,0,0,255};
    if(type==4||type==6) return {0,0,0,255};
    if(type==5) return {250,0,250,255};
    if (type==11) { return {255, 0, 0, 255};}
    if (type==10) { return {255, 128, 0, 255};}
     if(type>990) return {100,100,100,180};
    return {0,0,0,255};
}
int ContourData::getStyleByType(const int type){
    if(type==0) return 3;
    if(type==1) return 4;
    if(type==5) return 1;
    return 0;
}
float ContourData::getWidthByType(const int type){
    if(type==0||type==2) return 0.8;
    if(type>990) return 0.3; 
    return 0.4;
}
