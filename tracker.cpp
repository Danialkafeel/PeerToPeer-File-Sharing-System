#include <bits/stdc++.h> 
#include <unistd.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <thread> 
#define ll long long
using namespace std;

class myfile
{
public:
	string filename;
	ll f_size;
	vector<string> sha1;
	vector<pair<string,bool>> usersWithFiles;	// sharing or not
	myfile(string name, ll size){
		filename = name;
		f_size = size;
	}
	void add(string str){
		usersWithFiles.push_back(make_pair(str,1));
	}
	bool isUser(string usernm){
		for(auto s: usersWithFiles){
			if(s.first == usernm)
				return 1;
		}
		return 0;
	}
};
class user
{
public:
	// int peer_fd;
	string serving_ip,serving_port,username,password;
	bool isLoggedIn;
	unordered_map<string,string> fnameToPath;

	user(string uname,string passwd){
		// peer_fd = fd;
		username = uname;
		password = passwd;
		isLoggedIn = 0;
	}
	void login(string IP,string Port){
		isLoggedIn = 1;
		serving_ip = IP;
		serving_port = Port;
	}
	void logout(){
		serving_ip = "";
		serving_port = "";
		isLoggedIn = 0;
	}
};
class group
{
public:
	string groupID,owner;
	vector<string> members,filesinGroup, pendingRequests;
	group(string gid, string ownername){
		groupID = gid;
		owner = ownername;
		members.push_back(ownername);
	}
	bool isMember(string u){
		for(string s:members){
			if(s == u)
				return 1;
		}
		return 0;
	}
	bool isFile(string u){
		for(string s:filesinGroup){
			if(s == u)
				return 1;
		}
		return 0;
	}
	bool isReq(string m){
		for(string s:pendingRequests){
			if(s == m)
				return 1;
		}
		return 0;
	}
	void removeMember(string m){
		members.erase(find(members.begin(),members.end(),m));
	}
	void accept_req(string m){
		pendingRequests.erase(find(pendingRequests.begin(),pendingRequests.end(),m));
		members.push_back(m);
	}
};

unordered_map<string,user*> users;		// Username -> User ptr
unordered_map<string,myfile*> files;	// Filename -> file ptr
unordered_map<string,group*> groups;	// Groupname -> group ptr


bool userExists(string uname){
	if(users.find(uname) == users.end())
		return 0;
	return 1;
}

bool fileExists(string fname){
	if(files.find(fname) == files.end())
		return 0;
	return 1;
}

bool groupExists(string gname){
	if(groups.find(gname) == groups.end())
		return 0;
	return 1;
}

void clear_buff(char* buff,int len){
	for(int i=0;i<len;i++){
		buff[i] = '\0';
	}
}

void listening_peer(int peer_fd){
	cout<<"Listening to "<<peer_fd<<endl;
	while(1){
		char buff[100000];
		int bytes_read = read(peer_fd,buff,100000);
		if(bytes_read == 0){
			cout<<peer_fd<<" shutted down!\n";
			return;
		}
		cout<<"Received from "<<peer_fd<<": "<<buff<<endl;
		vector<string> cmds;
		char *token = strtok(buff, " ");
		while(token != NULL){
			string temp = token;
			cmds.push_back(token);
			token = strtok(NULL," ");
		}
		clear_buff(buff,100000);
		if(cmds[0] == "create_user"){
			if(userExists(cmds[1])){
				char msg[] = "User already exists";
				send(peer_fd, msg , strlen(msg) , 0 );
			}
			else{
				user* thisPeer = new user(cmds[1],cmds[2]);
				users[cmds[1]] = thisPeer;
				string msg = "Created "+cmds[1]+" Successfully!";
				send(peer_fd , msg.c_str() , msg.size() , 0 );
				cout<<"Created "<<cmds[1]<<"\n";
			}
		}
		if(cmds[0] == "check"){
			for(auto it = users.begin();it != users.end(); ++it){
				cout<<"Uname : "<<it->first<<", password = "<<it->second->password<<endl;
			}
			for(auto it = files.begin();it!= files.end(); ++it){
				cout<<"filename = "<<it->first<<" "<<it->second->f_size<<endl;
				cout<<"Users with this\n";
				for(auto s:it->second->usersWithFiles){
					cout<<s.first<<" , full path = "<<users[s.first]->fnameToPath[it->first]<<endl;
				}
			}

			string msg = "done!";
			send(peer_fd , msg.c_str() , msg.size() , 0 );	
		}
		if(cmds[0] == "logout"){
			if(!userExists(cmds[1])){
				char msg[] = "User doesn't exists";
				send(peer_fd, msg , strlen(msg) , 0 );
			}
			else{
				users[cmds[1]]->logout();
				char msg[] = "Logged Out!";
				send(peer_fd, msg , strlen(msg) , 0 );
			}
		}
		if(cmds[0] == "login"){
			if(!userExists(cmds[1])){
				char msg[] = "User does not exist";
				send(peer_fd, msg , strlen(msg) , 0 );
			}
			else{
				if(users[cmds[1]]->password != cmds[2]){
					char msg[] = "Incorrect Password";
					send(peer_fd, msg , strlen(msg) , 0 );
				}
				else{
					users[cmds[1]]->login(cmds[3],cmds[4]);
					string msg = "Loggedin ";
					for(auto itr = users[cmds[1]]->fnameToPath.begin(); itr != users[cmds[1]]->fnameToPath.end(); ++itr){
						msg += itr->first+" "+itr->second+" ";
					}
					send(peer_fd, msg.c_str() ,msg.size() , 0 );
				}
			}
		}

		if(cmds[0] == "create_grp"){
			if(!userExists(cmds[2])){
				char msg[] = "User does not exist";
				send(peer_fd, msg , strlen(msg) , 0 );
			}
			else if(groupExists(cmds[1])){
				char msg[] = "Group already exists";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
			else{
				group * ng = new group(cmds[1],cmds[2]);
				groups[cmds[1]] = ng;				
				char msg[] = "Group created successfully!";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
		}
		if(cmds[0] == "join_grp"){
			if(!userExists(cmds[2])){
				char msg[] = "User does not exist";
				send(peer_fd, msg , strlen(msg) , 0 );
			}
			else if(!groupExists(cmds[1])){
				char msg[] = "Group doesn't exist";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
			else if(groups[cmds[1]]->isMember(cmds[2])){
				char msg[] = "You are already a member of this group";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
			else{
				groups[cmds[1]]->pendingRequests.push_back(cmds[2]);
				char msg[] = "Request to join group sent!";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
		}
		if(cmds[0] == "leave_grp"){
			if(!userExists(cmds[2])){
				char msg[] = "User does not exist";
				send(peer_fd, msg , strlen(msg) , 0 );
			}
			else if(!groupExists(cmds[1])){
				char msg[] = "Group doesn't exist";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
			else if(!groups[cmds[1]]->isMember(cmds[2])){
				char msg[] = "You are not a member of this group";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
			else{
				groups[cmds[1]]->removeMember(cmds[2]);
				char msg[] = "You left the group!";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
		}
		if(cmds[0] == "list_requests"){
			if(!userExists(cmds[2])){
				char msg[] = "User does not exist\n";
				send(peer_fd, msg , strlen(msg) , 0 );
			}
			else if(!groupExists(cmds[1])){
				char msg[] = "Group doesn't exist\n";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
			else if(groups[cmds[1]]->owner != cmds[2]){
				char msg[] = "You are not the owner of this group\n";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
			else{
				string msg = "";
				for(string s: groups[cmds[1]]->pendingRequests)
					msg += s+"\n";
				if(msg == "")
					msg = "No pending requests\n";
				send(peer_fd, msg.c_str() , msg.size() , 0 );				
			}
		}
		if(cmds[0] == "accept_request"){
			if(!userExists(cmds[3])){
				char msg[] = "User does not exist";
				send(peer_fd, msg , strlen(msg) , 0 );
			}
			else if(!groupExists(cmds[1])){
				char msg[] = "Group doesn't exist";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
			else if(groups[cmds[1]]->owner != cmds[3]){
				char msg[] = "You are not the owner of this group";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
			else if(!groups[cmds[1]]->isReq(cmds[2])){
				char msg[] = "No pending request found for this user";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
			else{
				groups[cmds[1]]->accept_req(cmds[2]);
				char msg[] = "Request Accepted!";
				send(peer_fd, msg , strlen(msg) , 0 );
			}
		}
		if(cmds[0] == "list_grps"){
			string msg = "";
			for(auto it = groups.begin(); it != groups.end(); it++){
				msg += it->first+"\n";
			}
			if(msg == "")
				msg = "No Groups found!\n";
			send(peer_fd, msg.c_str() , msg.size() , 0 );
		}
		if(cmds[0] == "list_files"){
			if(!groupExists(cmds[1])){
				char msg[] = "Group doesn't exist\n";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
			else if(!groups[cmds[1]]->isMember(cmds[2])){
				char msg[] = "You are not a member of this group\n";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
			else{
				string msg = "";
				for(string s: groups[cmds[1]]->filesinGroup){
					for(auto uwf:files[s]->usersWithFiles){
						if(uwf.second == 1)
							msg += s+"\n";
					}
				}
				if(msg == "")
					msg = "No files in this group\n";
				send(peer_fd, msg.c_str() , msg.size() , 0 );				
			}
		}


		if(cmds[0] == "upload"){
			if(!userExists(cmds[2])){
				char msg[] = "User does not exist";
				send(peer_fd, msg , strlen(msg) , 0 );
			}
			if(!groupExists(cmds[1])){
				char msg[] = "Group doesn't exist";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
			else{
				// cout<<"jdskfjladf\n";
				// cout<<cmds[0]<<" "<<cmds[1]<<" "<<cmds[2]<<" "<<cmds[3]<<endl;
				if(!groups[cmds[1]]->isFile(cmds[3])){
					groups[cmds[1]]->filesinGroup.push_back(cmds[3]);
				}
				if(files.find(cmds[3]) != files.end()){
					if(files[cmds[3]]->isUser(cmds[2])){
						char msg[] = "File is already there.";
						send(peer_fd, msg , strlen(msg) , 0 );
					}
					else{
						files[cmds[3]]->add(cmds[2]);
						users[cmds[2]]->fnameToPath[cmds[3]] = cmds[4];
						string msg = cmds[3]+" uploaded successfully";
						send(peer_fd, msg.c_str() , msg.size() , 0 );						
					}		
				}
				else{
					myfile* temp = new myfile(cmds[3],stoi(cmds[5]));
					temp->add(cmds[2]);
					files[cmds[3]] = temp;
					users[cmds[2]]->fnameToPath[cmds[3]] = cmds[4];
					for(int i=6;i<cmds.size();i++){
						temp->sha1.push_back(cmds[i]);
					}
					string msg = cmds[3]+" uploaded successfully";
					send(peer_fd, msg.c_str() , msg.size() , 0 );
				}
			}
		}
		if(cmds[0] == "download"){
			string msg = "";
			string filetosearch = cmds[1];
			string grp = cmds[2];
			if(!groups[cmds[2]]->isMember(cmds[3])){
				char msg[] = "@";
				send(peer_fd, msg , strlen(msg) , 0 );
				continue;				
			}
			if(!groups[grp]->isFile(filetosearch)){
				msg = "~";
				send(peer_fd, msg.c_str(), msg.size(), 0);
				continue;
			}
			for(auto x : files[filetosearch]->sha1){
				msg += x+" ";
			}
			msg += "; ";
			for(auto t_user: files[filetosearch]->usersWithFiles){
				if(users[t_user.first]->isLoggedIn && t_user.second){	// User is logged in and sharing.
					msg += users[t_user.first]->serving_ip+" "+users[t_user.first]->serving_port+" ";
				}
			}
			send(peer_fd, msg.c_str(), msg.size(), 0);
		}
		if(cmds[0] == "stop_share"){
			if(!fileExists(cmds[2])){
				char msg[] = "File does not exist";
				send(peer_fd, msg , strlen(msg) , 0 );
			}
			else if(!groupExists(cmds[1])){
				char msg[] = "Group doesn't exist";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
			else if(!groups[cmds[1]]->isMember(cmds[3])){
				char msg[] = "You are not a member of this group";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
			else{
				for(auto x :files[cmds[2]]->usersWithFiles){
					if(x.first == cmds[3]){
						x.second = false;
						break;
					}
				}
				char msg[] = "Stopped sharing";
				send(peer_fd, msg , strlen(msg) , 0 );				
			}
		}

		// if(cmds[0] == "show"){
		// 	if(!userExists(cmds[1])){
		// 		char msg[] = "User does not exist";
		// 		send(peer_fd, msg , strlen(msg) , 0 );
		// 	}
		// 	else{
		// 		string msg = "";
		// 		// for(auto s:users[cmds[1]]->files){
		// 		// 	msg += "[C] "+s.filename+"\n";
		// 		// }
		// 		// cout<<"msg = "<<msg<<endl;
		// 		send(peer_fd, msg.c_str() , msg.size() , 0 );
		// 	}
		// }
	}
}

void quit(bool &isquit){
	string str;
	cin>>str;
	if(str == "quit"){
		// cout<<"changed\n";
		isquit = 1;
		return;
	}
}


int main(int argc, char const *argv[]) 
{ 
	int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 

	if(argc != 2){
		cout<<"Invalid args\n";
	}

	fstream file;
	file.open(argv[1]);
	string tracker_ip,tracker_port;
	file >> tracker_ip;
	file >> tracker_port;
	cout<<"Serving at "<<tracker_ip<<":"<<tracker_port<<endl;
	
	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){ 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt))){ 
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	// inet_aton(tracker_ip.c_str(), &address.sin_addr);
	address.sin_port = htons(stoi(tracker_port)); 
	
	// Forcefully Binding socket to the given port 
	if (bind(server_fd, (struct sockaddr *)&address,sizeof(address))<0){ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	}
	if (listen(server_fd, 15) < 0){ 
		perror("listen");
		exit(EXIT_FAILURE);
	}

	bool isquit = 0;
	thread to_exit(quit,ref(isquit));

	vector<thread> peers_thread;
	int peers_thread_ind = 0;
	while(1){
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0){
			perror("accept");
			exit(EXIT_FAILURE); 
		}
		cout<<"new_socket Created : "<<new_socket<<endl;
		peers_thread.push_back(thread(listening_peer,new_socket));
		// cout<<"isquit = "<<isquit<<endl;
		// if(isquit){
		// 	cout<<"Shutting down Tracker\n";
		// 	return 1;
		// }
	}
	cout<<"peers_thread size = "<<peers_thread.size()<<endl;
	for(int i=0;i<peers_thread.size();i++){
		peers_thread[i].join();
	}
	return 0; 
} 
