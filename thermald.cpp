#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ini.h>
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <fstream>
#include <sstream>

class thermaction {
	public:
		int limit;
		int type;//-1 lowing, 1 highing 0 any
		int acttype;//ignored
		std::string path;
		std::string value;
		bool wall_enabled;
};
class thermalzone {
	public:
		std::string path;
		std::map<std::string, thermaction> events;
		int polling_interval = 100;//ignored
		int lastvalue;
};

class configholder {
	public:
		std::map<std::string, thermalzone> zones;
};

int read_config(void* cfg, const char *section, const char *name, const char *value) {
	configholder* cf = (configholder*)cfg;
	std::string sect = section;
	std::string key = name;
	thermalzone& z = cf->zones[sect];
	if (key=="path") {
		z.path = value;
	}
	size_t ppos = key.find(".");
	if (ppos!=std::string::npos) {
		thermaction& ta = z.events[key.substr(0,ppos)];
		std::string ekey = key.substr(ppos+1, std::string::npos);
		if (ekey=="limit") ta.limit = atoi(value);
		if (ekey=="type") ta.type = atoi(value);
		if (ekey=="path") ta.path = value;
		if (ekey=="value") ta.value = value;
		if (ekey=="wall"&&strcmp(value, "no")!=0) ta.wall_enabled = true;
	}
}

int readfvalue(std::string fname) {
	std::ifstream ifile;
	ifile.open(fname);
	int value;
	ifile >> value;
	return value;
}

void writefvalue(std::string fname, std::string value) {
	std::ofstream ofile;
	ofile.open(fname);
	ofile<<value;
}


int main() {
	configholder cf;
	ini_parse("thermald.conf", read_config, &cf); 	
	for(auto const &ent: cf.zones) {
		std::cout << ent.first << "\n";
		std::cout << ent.second.path << "\n";
		for(auto const &ente: ent.second.events) {
			std::cout << ente.first << "\n";
			std::cout << ente.second.limit << "\n";
			std::cout << ente.second.type << "\n";
			std::cout << ente.second.path << "\n";
			std::cout << ente.second.value << "\n";	
		}
	}
	std::cout << "starting loop\n";
	while(1) {
		usleep(100000);
		for(auto &ent: cf.zones) {
			int cvalue = readfvalue(ent.second.path);
			int lvalue = ent.second.lastvalue;
			ent.second.lastvalue = cvalue;
			if (cvalue==lvalue) {
				continue;
			}
			std::cout << ent.first << " " << cvalue << "\n";
			for(auto &ente: ent.second.events) {
				thermaction& act = ente.second;
				int direction=0;
				if (lvalue==0) {
					if (cvalue>act.limit) {
						direction = 1;
					}
					if (cvalue<act.limit) {
						direction = -1;
					}
				} else {
					if (lvalue<=act.limit&&cvalue>act.limit) {
						direction = 1;
					}
					if (lvalue>=act.limit&&cvalue<act.limit) {
						direction = -1;
					}
				}
				if (direction!=0) {
					std::cout << ente.first << " passed dir. " << direction << "\n";
					if (direction == ente.second.type) {
						writefvalue(ente.second.path, ente.second.value);
						std::cout << ente.second.value << " >> " << ente.second.path << "\n";
						if (ente.second.wall_enabled) {
							std::ostringstream wallcmd;
							wallcmd << "wall \"" << "point " << ente.first << 
								" passed at temp " << cvalue << 
								" switch to " << ente.second.value << "\"";
							std::string wallcmdstr = wallcmd.str();
							system(wallcmdstr.c_str());
						}
					}
				}
			}
		}
	}
}
