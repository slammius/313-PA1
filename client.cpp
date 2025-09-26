/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Bryson Tran
	UIN: 733007479
	Date: 9/24/2025
*/

#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>

using namespace std;

int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	string fname = "";
	int message_capacity = MAX_MESSAGE;
	vector<FIFORequestChannel*> channels;

	bool input_p = false; 
	bool input_t = false;
	bool input_e = false; 
	bool input_f = false;
	bool input_c = false;

	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p': 
				p = atoi(optarg); 
				input_p = true; 
				break;
			case 't': 
				t = atof(optarg); 
				input_t = true; 
				break;
			case 'e': 
				e = atoi(optarg); 
				input_e = true; 
				break;
			case 'f': 
				fname = optarg; 
				input_f = true; 
				break;
			case 'm': 
				message_capacity = atoi(optarg); 
				break;
			case 'c': 
				input_c = true; 
				break;
		}
	}

	// 4.1 
	pid_t pid = fork();
	if (pid == 0) {
		if (message_capacity != MAX_MESSAGE) {
			string message = to_string(message_capacity);
			execl("./server", "./server", "-m", message.c_str(), nullptr);
		} 
		else {
			execl("./server", "./server", nullptr);
		}
	}
    FIFORequestChannel* control_channel = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    channels.push_back(control_channel);
    FIFORequestChannel* channel = channels.back();

	// 4.4 
	if (input_c) {
		MESSAGE_TYPE channel_message = NEWCHANNEL_MSG;
		control_channel->cwrite(&channel_message, sizeof(MESSAGE_TYPE));

		char channel_name[20] = {};
		control_channel->cread(channel_name, sizeof(channel_name));
		FIFORequestChannel* channel2 = new FIFORequestChannel(channel_name, FIFORequestChannel::CLIENT_SIDE);
		channels.push_back(channel2);
    	channel = channel2;
	}

	// 4.2 
	if (input_p && input_t && input_e) {
		datamsg datapoint(p, t, e);
		channel->cwrite(&datapoint, sizeof(datamsg));
		double ecg_val;
		channel->cread(&ecg_val, sizeof(double));

		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << ecg_val << endl;
	} 
	else if (input_p && !input_t && !input_e) {
		ofstream ofs("received/x1.csv");
		for (int i = 0; i < 1000; i++) {
			double time_val = i * 0.004;

			datamsg datapoint1(p, time_val, 1);;
			channel->cwrite(&datapoint1, sizeof(datamsg));
			double ecg_val1;
			channel->cread(&ecg_val1, sizeof(double));

			datamsg datapoint2(p, time_val, 2);;
			channel->cwrite(&datapoint2, sizeof(datamsg));
			double ecg_val2;
			channel->cread(&ecg_val2, sizeof(double));

			ofs << time_val << "," << ecg_val1 << "," << ecg_val2 << "\n";
		}
	}

	// 4.3 
	if (input_f) {
		ifstream ifs(fname);
    	ofstream ofs("BIMDC/" + fname);
    	ofs << ifs.rdbuf(); 

		filemsg fm(0, 0);
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buffer = new char[len];                          
		memcpy(buffer, &fm, sizeof(filemsg));
		strcpy(buffer + sizeof(filemsg), fname.c_str());
		channel->cwrite(buffer, len);
		delete[] buffer;                                        
		__int64_t file_size = 0;
		channel->cread(&file_size, sizeof(__int64_t));

		ofstream ofs2(string("received/") + fname);
		__int64_t offset = 0;
		while (offset < file_size) {
	    	__int64_t chunk;
    		if (file_size - offset < message_capacity) {
				chunk = file_size - offset;
    		} 
			else {
        		chunk = message_capacity;
    		}

			filemsg fm(offset, chunk);
			char* buffer2 = new char[len];                    
			memcpy(buffer2, &fm, sizeof(filemsg));
			strcpy(buffer2 + sizeof(filemsg), fname.c_str());
			channel->cwrite(buffer2, len);
			delete[] buffer2;                                

			char* chunk_buffer = new char[chunk];                    
			int bytes = channel->cread(chunk_buffer, chunk);
			ofs2.seekp(offset);
			ofs2.write(chunk_buffer, bytes);
			offset += bytes;
			delete[] chunk_buffer;                                   
		}
	}

	// 4.5 
	for (size_t i = 0; i < channels.size(); i++) {
		MESSAGE_TYPE m = QUIT_MSG;
		channels[i]->cwrite(&m, sizeof(MESSAGE_TYPE));
		delete channels[i];
	}
	wait(nullptr);
}
