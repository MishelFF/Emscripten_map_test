
#include "wellData.h"
#include "strFunctions.h"

int WellData::loadWellsFile(const char* filename) {
    std::ifstream file(filename);
    std::string line;
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return 1; 
    }
    wells.clear();
    WellInfo current_well;
    std::string buf;
    while (std::getline(file,line)){
        std::stringstream ss(line);
        fontSprite current_sprite;
        current_well.sprites.clear();
        current_well.labels.clear();
        std::getline(ss, current_well.NC, ';');
        std::getline(ss, current_well.clust, ';');
        std::getline(ss, buf, ';');current_well.coord.x=std::stod(buf);
        std::getline(ss, buf, ';');current_well.coord.y=std::stod(buf);
        std::getline(ss, buf, ';');current_well.bot.x=std::stod(buf);
        std::getline(ss, buf, ';');current_well.bot.y=std::stod(buf);
        std::getline(ss, buf, ';');current_sprite.code=std::stoi(buf);
        std::getline(ss, buf, ';');current_sprite.color=std::stoi(buf);
        std::getline(ss, buf, ';');current_sprite.center_color=std::stoi(buf);
        current_well.sprites.push_back(current_sprite);
        for (int i=0;i<3;i++) {
            std::getline(ss, buf, ';');
            buf=trim(buf);
            if (buf.size()>0) current_well.labels.push_back(buf);
        }
        std::getline(ss, buf, ';');
        std::getline(ss, buf, ';');current_well.current_dev.dn=std::stod(buf);
        std::getline(ss, buf, ';');current_well.current_dev.dw=std::stod(buf);
        std::getline(ss, buf, ';');current_well.current_dev.nw=std::stod(buf);
        std::getline(ss, buf, ';');current_well.current_dev.ng=std::stod(buf);
        std::getline(ss, buf, ';');current_well.current_dev.hourWorkInput=std::stoi(buf);
        std::getline(ss, buf, ';');current_well.current_dev.hourWorkOutput=std::stoi(buf);
        std::getline(ss, buf, ';');std::getline(ss, buf, ';');
        std::getline(ss, buf, ';');current_well.NCColor=std::stoi(buf);
        std::getline(ss, buf, ';');
        std::getline(ss, buf, ';');current_well.typeRef=std::stoi(buf);
        std::getline(ss, buf, ';');current_well.stateRef=std::stoi(buf);

        wells.push_back(current_well);
    }
    file.close();
    return 0; 
}