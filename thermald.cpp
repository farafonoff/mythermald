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

std::ofstream log;

void initlog() {
	log.open("thermald.log", std::ofstream::out | std::ofstream::app);
}

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

#define BUFSIZE 100
char buffer[BUFSIZE];
int readfvalue_c(std::string fname) {
	FILE *fs = fopen(fname.c_str(), "r");
	fgets(buffer, BUFSIZE, fs);
	int result = atoi(buffer);
	fclose(fs);
	return result;
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
	daemon(1,0);
	initlog();
	configholder cf;
	ini_parse("thermald.conf", read_config, &cf); 	
	for(auto const &ent: cf.zones) {
		log << ent.first << std::endl;
		log << ent.second.path << std::endl;
		for(auto const &ente: ent.second.events) {
			log << ente.first << std::endl;
			log << ente.second.limit << std::endl;
			log << ente.second.type << std::endl;
			log << ente.second.path << std::endl;
			log << ente.second.value << std::endl;	
		}
	}
	log << "starting loop\n";
	while(1) {
		usleep(200000);
		for(auto &ent: cf.zones) {
			int cvalue = readfvalue_c(ent.second.path);
			int lvalue = ent.second.lastvalue;
			ent.second.lastvalue = cvalue;
			if (cvalue==lvalue) {
				continue;
			}
			log << ent.first << " " << cvalue << std::endl;
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
					log << ente.first << " passed dir. " << direction << std::endl;
					if (direction == ente.second.type) {
						writefvalue(ente.second.path, ente.second.value);
						log << ente.second.value << " >> " << ente.second.path << std::endl;
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

