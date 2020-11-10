#include <bits/stdc++.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <thread>
#include <openssl/sha.h>
#include <fstream>
#define ll long long
#define KB(x)   ((size_t) (x) << 10)

using namespace std;

bool isLoggedIn;
string uname;
unordered_map<string,string> fnameToPath;
vector<string> downloading;
int tracker_sock;

void logged_out(){
	uname = "";
	isLoggedIn = 0;
	fnameToPath.clear();
}
void logged_in(string str){
	uname = str;
	isLoggedIn = 1;
}

// class File
// {
// public:
// 	string f_name,filePath;
// 	ll f_size, no_of_chunks;
// 	bool sharing;
// 	vector<string> sha1;
// 	File(string name,string path,ll size){
// 		f_name = name;
// 		f_size = size;
// 		filePath = path;
// 		no_of_chunks = ceil((double)size/KB(512));
// 		sharing = 1;
// 	}
// 	void stop_sharing(){
// 		sharing = 0;
// 	}
	
// };
// unordered_map<string,File*> myfiles;

void clear_buff(char* buff,int len){
	for(int i=0;i<len;i++){
		buff[i] = '\0';
	}
}

void serving_util(int new_socket){
	cout<<"Yo\n";
	char buff[10000];
	int bytes_read = read(new_socket,buff,10000);
	if(bytes_read == 0){
		cout<<"Peer disconnected inexpectedly\n";
	}
	cout<<"read in serving_util "<<buff<<endl;
	vector<string> cmds;
	char *token = strtok(const_cast<char*>(buff), " ");
	while(token != NULL){
		string temp = token;
		cmds.push_back(token);
		token = strtok(NULL," ");
	}
	if(cmds[0] == "get"){
		string req_file = fnameToPath[cmds[1]];
		int chunk_no = stoi(cmds[2]);
		ifstream fin;
		fin.open(req_file);
		fin.seekg(KB(512)*(chunk_no-1));
		size_t chunk_size = KB(512);
		char * chunk = new char[chunk_size];
		fin.read(chunk,chunk_size);
		cout<<"actually read = "<<fin.gcount()<<endl;
		string size_of_chunk = to_string(fin.gcount());
		char to_send[fin.gcount()];
		for(int i=0;i<fin.gcount();i++){
			to_send[i] = chunk[i];
		}
		send(new_socket,size_of_chunk.c_str(),size_of_chunk.size(),0);
		send(new_socket,to_send,fin.gcount(),0);
	}
	else{
		cout<<"Invalid msg from peer\n";
	}
}

void serve_to_peers(string IP,string port){
	struct  sockaddr_in addr;
	int sock_fd, opt = 1;
	if((sock_fd = socket(AF_INET,SOCK_STREAM,0)) == 0){
		perror("Socket failed");
		return;
	}
	if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt))){ 
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(stoi(port)); 
	if (bind(sock_fd, (struct sockaddr *)&addr,sizeof(addr))<0){ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	}
	if (listen(sock_fd, 15) < 0){ 
		perror("listen");
		exit(EXIT_FAILURE);
	}
	int addrlen = sizeof(addr);
	int new_socket;
	vector<thread> servingPeerThreads;
	while(1){
		if ((new_socket = accept(sock_fd, (struct sockaddr *)&addr,(socklen_t*)&addrlen))<0){
			perror("accept");
			exit(EXIT_FAILURE); 
		}
		cout<<"connected\n";
		servingPeerThreads.push_back(thread(serving_util,new_socket));

	}

}
void download_chunk(string file_name,string sha1_of_this_chunk,int chunk_no, string IP, string port,string dest){
	int peer_sock = 0;
	struct sockaddr_in serv_addr; 
	if ((peer_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){ 
		printf("\n Socket creation error \n"); 
		return ; 
	} 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(stoi(port.c_str())); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, IP.c_str(), &serv_addr.sin_addr)<=0) { 
		printf("\nInvalid address/ Address not supported \n"); 
		return; 
	}
	if (connect(peer_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){ 
		printf("\nConnection Failed \n"); 
		return; 
	}
	string msg = "get "+file_name+" "+to_string(chunk_no);
	send(peer_sock,msg.c_str() ,msg.size(),0);
	char temp[20];
	int size_of_chunk_recvd = read(peer_sock,temp,20);
	string size_of_chunk = temp;
	int size_ofchunk = stoi(size_of_chunk);
	char buff[size_ofchunk];
	clear_buff(buff,size_ofchunk);
	
	int bytes_read = read(peer_sock,buff,size_ofchunk);
	cout<<"bytes_read = "<<bytes_read<<endl;
	// cout<<"buff = "<<buff<<endl;
	ofstream outfile;
	cout<<"writing chunk_no "<<chunk_no<<endl;
	if(chunk_no == 1){
		outfile.open(dest);
		outfile.write(buff,size_ofchunk);		
	}
	else{
		outfile.open(dest,ios_base::app);
		outfile.write(buff,size_ofchunk);
	}
	outfile.close();

}

void download(string file_name,vector<string> original_sha1,vector<pair<string,string>>ip_ports,string dest_path){
	int no_of_chunks = original_sha1.size();
	cout<<"no_of_chunks = "<<no_of_chunks<<endl;
	int peers_available = ip_ports.size();
	cout<<"peers_available for this file = "<<peers_available<<endl;
	cout<<"Downloading... "<<file_name<<endl;
	downloading.push_back(file_name);

	if(no_of_chunks <= peers_available){
		// cout<<"Yo\n";
		vector<thread> fetch_chunks;
		for(int i = 1; i <= no_of_chunks; i++){
			fetch_chunks.push_back(thread(download_chunk,file_name,original_sha1[i-1], i , ip_ports[i-1].first,ip_ports[i-1].second,dest_path));
		}
		while(fetch_chunks.size() > 0){
			fetch_chunks[0].join();
			fetch_chunks.erase(fetch_chunks.begin());
		}
	}
	else{
		vector<thread> fetch_chunks;
		int n_from_each = no_of_chunks/peers_available;
		int chunk_no = 1;
		for(int i=1;i<=peers_available;i++){
			for(int j=0;j<n_from_each;j++){
				cout<<"chunk_no = "<<chunk_no<<endl;
				fetch_chunks.push_back(thread(download_chunk,file_name,original_sha1[chunk_no-1], chunk_no , ip_ports[i-1].first,ip_ports[i-1].second,dest_path));
				chunk_no++;
			}
		}
		int remaining = no_of_chunks % peers_available;
		for(int i = 0; i<remaining;i++){
			fetch_chunks.push_back(thread(download_chunk,file_name,original_sha1[chunk_no-1], chunk_no , ip_ports[i].first,ip_ports[i].second,dest_path));
			chunk_no++;			
		}

		while(fetch_chunks.size() > 0){
			fetch_chunks[0].join();
			fetch_chunks.erase(fetch_chunks.begin());
		}
	}

	downloading.erase(find(downloading.begin(),downloading.end(),file_name));
	cout<<"Download Complete for "<<file_name<<endl;
	fnameToPath[file_name] = dest_path;
	// send(tracker_sock,"download_completed ");	//I have downloaded this file.
	return;
}


string send_message(int sock, string str){
	send(sock , str.c_str() , str.size() , 0 );
	char buff[1000];
	clear_buff(buff,1000);
	int bytes_read = read(sock,buff,1000);
	string ans = buff;
	return ans;
}

ll findSize(const char file_name[]) 
{ 
	FILE* fp = fopen(file_name, "r");   
	if (fp == NULL) { 
	    printf("File Not Found!\n"); 
	    return -1; 
	}
	fseek(fp, 0L, SEEK_END); 
	long int res = ftell(fp);   
	fclose(fp);   
	return res; 
}

string getFileName(string path){
	int n = path.size();
	int s = n-1;
	while(path[s--] != '/');
	s+=2;
	string ans = "";
	while(s<n){
		ans += path[s++];
	}
	return ans;
}


int main(int argc, char const *argv[]) 
{ 
	if(argc != 3){
		cout<<"Invalid Args\n";
		return 1;
	}
	// isLoggedIn = 0;
	logged_out();

	string client_ip, client_port, tracker_ip, tracker_port;
	int ind = 0;
	while(argv[1][ind] != ':'){
		client_ip.push_back(argv[1][ind++]);
	}
	ind++;
	while(argv[1][ind] != '\0'){
		client_port.push_back(argv[1][ind++]);
	}
	fstream file;
	file.open(argv[2]);
	file >> tracker_ip;
	file >> tracker_port;
	// cout<<client_ip<<":"<<client_port<<" "<<tracker_ip<<":"<<tracker_port<<endl;

	tracker_sock = 0; 
	struct sockaddr_in serv_addr; 
	if ((tracker_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){ 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(stoi(tracker_port.c_str())); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, tracker_ip.c_str(), &serv_addr.sin_addr)<=0) { 
		printf("\nInvalid address/ Address not supported \n"); 
		return -1; 
	}
	if (connect(tracker_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){ 
		printf("\nConnection Failed \n"); 
		return -1; 
	}
	thread th(serve_to_peers,client_ip,client_port);
	while(1){
		string cmd;
		getline(cin,cmd);
		// cout<<"cmd = "<<cmd<<endl;
		vector<string> cmds;
		char *token = strtok(const_cast<char*>(cmd.c_str()), " ");
		while(token != NULL){
			string temp = token;
			cmds.push_back(token);
			token = strtok(NULL," ");
		}
		// for(string s:cmds)
		// 	cout<<s<<" ";
		if(cmds.size() == 0)
			continue;

		if(cmds[0] == "create_user"){
			if(cmds.size() != 3){
				cout<<"Invalid args to create_user\n";
				continue;
			}
			string reply = send_message(tracker_sock,"create_user "+cmds[1]+" "+cmds[2]);
			cout<<reply<<endl;
		}
		else if(cmds[0] == "check"){
			string reply = send_message(tracker_sock,"check");
			cout<<reply<<endl;
		}
		else if(cmds[0] == "login"){
			if(cmds.size() != 3){
				cout<<"Invalid args to login\n";
				continue;
			}
			if(isLoggedIn){
				cout<<"Logging out "<<uname<<endl;
				string reply = send_message(tracker_sock,"logout "+uname);
				cout<<reply<<endl;
				logged_out();
			}
			string reply = send_message(tracker_sock,"login "+cmds[1]+" "+cmds[2]+" "+client_ip+" "+client_port);
			if(reply[0] == 'L'){
				logged_out();
				logged_in(cmds[1]);
				cout<<cmds[1]<<" logged in!"<<endl;
				vector<string> reply_tokens;
				char *token = strtok(const_cast<char*>(reply.c_str()), " ");
				while(token != NULL){
					string temp = token;
					reply_tokens.push_back(token);
					token = strtok(NULL," ");
				}
				for(int i=1;i<reply_tokens.size();i+=2){
					cout<<reply_tokens[i]<<" -> "<<reply_tokens[i+1]<<endl;
					fnameToPath[reply_tokens[i]] = reply_tokens[i+1];
				}
			}
			else
				cout<<reply<<endl;
		}
		else if(cmds[0] == "logout"){
			if(cmds.size() != 1){
				cout<<"Invalid args to <logout>\n";
				continue;
			}
			if(!isLoggedIn){
				cout<<"No User Logged in\n";
				continue;
			}
			string reply = send_message(tracker_sock,"logout "+uname);
			cout<<reply<<endl;
			logged_out();
		}

		else if(cmds[0] == "create_group"){
			if(cmds.size() != 2){
				cout<<"Invalid args to <create_group>\n";
				continue;
			}
			if(!isLoggedIn){
				cout<<"No User Logged in\n";
				continue;
			}
			string reply = send_message(tracker_sock,"create_grp "+cmds[1]+" "+uname);
			cout<<reply<<endl;
		}
		else if(cmds[0] == "join_group"){
			if(cmds.size() != 2){
				cout<<"Invalid args to <join_group>\n";
				continue;
			}
			if(!isLoggedIn){
				cout<<"No User Logged in\n";
				continue;
			}
			string reply = send_message(tracker_sock,"join_grp "+cmds[1]+" "+uname);
			cout<<reply<<endl;			
		}
		else if(cmds[0] == "leave_group"){
			if(cmds.size() != 2){
				cout<<"Invalid args to <leave_group>\n";
				continue;
			}
			if(!isLoggedIn){
				cout<<"No User Logged in\n";
				continue;
			}
			string reply = send_message(tracker_sock,"leave_grp "+cmds[1]+" "+uname);
			cout<<reply<<endl;			
		}
		else if(cmds[0] == "list_requests"){
			if(cmds.size() != 2){
				cout<<"Invalid args to <list_requests>\n";
				continue;
			}
			if(!isLoggedIn){
				cout<<"No User Logged in\n";
				continue;
			}
			string reply = send_message(tracker_sock,"list_requests "+cmds[1]+" "+uname);
			cout<<reply;			
		}
		else if(cmds[0] == "accept_request"){
			if(cmds.size() != 3){
				cout<<"Invalid args to <accept_request>\n";
				continue;
			}
			if(!isLoggedIn){
				cout<<"No User Logged in\n";
				continue;
			}
			string reply = send_message(tracker_sock,"accept_request "+cmds[1]+" "+cmds[2]+" "+uname);
			cout<<reply<<endl;			
		}
		else if(cmds[0] == "list_groups"){
			if(cmds.size() != 1){
				cout<<"Invalid args to <list_groups>\n";
				continue;				
			}
			if(!isLoggedIn){
				cout<<"No User Logged in\n";
				continue;
			}
			string reply = send_message(tracker_sock,"list_grps");
			cout<<reply;						
		}
		else if(cmds[0] == "list_files"){
			if(cmds.size() != 2){
				cout<<"Invalid args to <list_files>\n";
				continue;				
			}
			if(!isLoggedIn){
				cout<<"No User Logged in\n";
				continue;
			}
			string reply = send_message(tracker_sock,"list_files "+cmds[1]+" "+uname);
			cout<<reply;						
		}
		

		else if(cmds[0] == "upload_file"){
			if(cmds.size() != 3){
				cout<<"Invalid args to <upload_file>\n";
				continue;
			}
			if(!isLoggedIn){
				cout<<"No User Logged in\n";
				continue;
			}
			string filePath = cmds[1];
			ll file_size = findSize(filePath.c_str());
			if(file_size != -1){
				cout<<"File size = "<<file_size<<endl;
				size_t chunk_size = KB(512);
				cout<<"chunk_size = "<<chunk_size<<endl;
				char * chunk = new char[chunk_size];
				string file_name = getFileName(filePath);
				fnameToPath[file_name] = filePath;
				ifstream fin;
				fin.open(filePath);
				vector<string> sha1;
				while(fin){
					fin.read(chunk,chunk_size);
					if(!fin.gcount()){
						break;
					}
					unsigned char hash[SHA_DIGEST_LENGTH]; // == 20
					SHA1(reinterpret_cast<const unsigned char *>(chunk), sizeof(chunk) - 1, hash);
					string c_hash(reinterpret_cast<char*>(hash));
					sha1.push_back(c_hash);
					cout<<"Pushed\n";
					cout<<hash<<endl;
				}
				string msg="upload "+cmds[2]+" "+uname+" "+file_name+" "+filePath+" "+to_string(file_size)+" ";
				for(string s:sha1){
					msg += s+" ";
				}
				cout<<send_message(tracker_sock,msg)<<endl;
			}
		}
		else if(cmds[0] == "download_file"){
			if(cmds.size() != 4){
				cout<<"Invalid args to <download_file>\n";
				continue;
			}
			if(!isLoggedIn){
				cout<<"No User Logged in\n";
				continue;
			}
			string file_name = cmds[2], dest_path = cmds[3];
			string fileNpeersinfo = send_message(tracker_sock,"download "+file_name+" "+cmds[1]+" "+uname);
			// cout<<fileNpeersinfo<<endl;
			if(fileNpeersinfo[0] == '~'){
				cout<<"File not found in the Group\n";
				continue;
			}
			if(fileNpeersinfo[0] == '@'){
				cout<<"You are not a member of the group\n";
				continue;
			}
			vector<string> original_sha1;
			vector<pair<string,string>> ip_ports;

			vector<string> reply_tokens;
			char *token = strtok(const_cast<char*>(fileNpeersinfo.c_str()), " ");
			while(token != NULL){
				string temp = token;
				reply_tokens.push_back(token);
				token = strtok(NULL," ");
			}
			int ind = 0;
			while(reply_tokens[ind] != ";"){
				// fnameToPath[reply_tokens[i]] = reply_tokens[i+1];
				original_sha1.push_back(reply_tokens[ind]);
				ind++;
			}
			ind++;
			for(;ind<reply_tokens.size();ind+=2){
				ip_ports.push_back(make_pair(reply_tokens[ind],reply_tokens[ind+1]));
			}
			if(ip_ports.size() == 0){
				cout<<"No peer is serving this file currently\n";
				continue;
			}
			cout<<"original_sha1\n";
			for(string s: original_sha1)
				cout<<s<<endl;
			cout<<"ip_ports\n";
			for(auto s:ip_ports){
				cout<<s.first<<" "<<s.second<<endl;
			}
			vector<thread> curr_downloaders;
			curr_downloaders.push_back(thread(download,file_name,original_sha1,ip_ports,dest_path));
			while(curr_downloaders.size() > 0){
				curr_downloaders[0].join();
				curr_downloaders.erase(curr_downloaders.begin());
				// cout<<"size = "<<curr_downloaders.size()<<endl;
			}

		}
		else if(cmds[0] == "show_downloads"){
			if(cmds.size() != 1){
				cout<<"Invalid args to <show_downloads>\n";
				continue;
			}
			if(!isLoggedIn){
				cout<<"No User Logged in\n";
				continue;
			}
			for(auto it = fnameToPath.begin();it != fnameToPath.end();it++){
				cout<<"[C] "<<it->first<<endl;
			}
			for(auto s: downloading){
				cout<<"[D] "<<s<<endl;
			}
		}
		else if(cmds[0] == "stop_share"){
			if(cmds.size() != 3){
				cout<<"Invalid args to <stop_share>\n";
				continue;
			}
			if(!isLoggedIn){
				cout<<"No User Logged in\n";
				continue;
			}
			string reply = send_message(tracker_sock,"stop_share "+cmds[1]+" "+cmds[2]+" "+uname);
			cout<<reply<<endl;
		}

		else{
			cout<<"Invalid Command\n";
		}
	}
	th.join();
	return 0; 
} 
