#include <bits/stdc++.h>
#include <iostream>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
using namespace std;


// Color codes
const std::string red("\033[0;31m");
const std::string green("\033[1;32m");
const std::string yellow("\033[1;33m");
const std::string cyan("\033[0;36m");
const std::string magenta("\033[0;35m");
const std::string reset("\033[0m");
// End of color codes


vector<string> cmd;

class user
{
public:
    string user_id;
    string pass;
    bool isonline;
    string ip;
    string port;
    map<string,bool>issharable;
    vector<string> ownerof;
    vector<string> memberof;

    user(string userid, string passs)
    {
        user_id = userid;
        pass = passs;
        isonline = false;
    }
};

class File
{
public:
    string path;
    string totalchunks;
    string sha1;
    //    map<int,vector<pair<string,string>>>chunkholder;   //chunkno and members conataining that chunk (vector of pair containing peer ip and port)
    map<int, vector<user *>> chunkholder;
    map<user *, int> peertotalchunk; //tell us about how many chunks of that file a peer is holding right now.
    File(string filepath, string totchunks)
    {
        path = filepath;
        totalchunks = totchunks;
    }
};

map<string, File *> files;

class Group
{
public:
    string gid;
    string owner;
    vector<string> members;
    vector<string> pending_members;
    // vector<string>files;
    map<string, string> files; //path,filename
    Group(string groupid, string newowner)
    {
        gid = groupid;
        owner = newowner;
        members.push_back(newowner);
    }
};

map<string, user *> users;   //uid,object
map<string, Group *> groups; //gid,object

static const int K_READ_BUF_SIZE{1024 * 16};
string getsha1(string filename)
{
    SHA256_CTX context;
    if (!SHA256_Init(&context))
    {
        return "false";
    }

    char buf[K_READ_BUF_SIZE];
    std::ifstream file(filename, std::ifstream::binary);
    while (file.good())
    {
        file.read(buf, sizeof(buf));
        if (!SHA256_Update(&context, buf, file.gcount()))
        {
            return "false";
        }
    }

    unsigned char result[SHA256_DIGEST_LENGTH];
    if (!SHA256_Final(result, &context))
    {
        return "false";
    }

    std::stringstream shastr;
    shastr << std::hex << std::setfill('0');
    for (const auto &byte : result)
    {
        shastr << std::setw(2) << (int)byte;
    }
    return shastr.str();
    // cout<<shastr.str()<<endl;
}

void create_user_fun(string userid, string pass, int c_socket)
{

    if (users.find(userid) == users.end())
    {
        user *newuser = new user(userid, pass);
        users[userid] = newuser;
        char buf[1024] = "true";
        send(c_socket, buf, sizeof(buf), 0);
        cout << "::>> User with Userid: \""<<userid<<"\", Registered!\n";
    }
    else
    {
        char buf[1024] = "false";
        send(c_socket, buf, sizeof(buf), 0);
        cout << "::>> User with Userid: \""<<userid<<"\", Already Present.\n";
    }
}

void login_fun(string userid, string pass1, string peerip, string peerport, int c_socket)
{

    if (users.find(userid) != users.end())
    {

        user *thatuser = users[userid];
        string userpass = thatuser->pass;

        if (userpass == pass1)
        {
            thatuser->isonline = true;
            thatuser->ip = peerip;
            thatuser->port = peerport;

            for(auto i : thatuser->issharable)  // again enabling all Sharing.
            {
                i.second=true;
            }

            char buf[1024] = "true";
            send(c_socket, buf, sizeof(buf), 0);
            cout<<"::>> User "<<userid<<" Logged in.\n";
        }
        else
        {
            char buf[1024] = "false1";
            send(c_socket, buf, sizeof(buf), 0);
            cout<<"::>> Invalid Password for "<<userid<<" User Account \n";
        }
    }
    else
    {
        char buf[1024] = "false2";
        send(c_socket, buf, sizeof(buf), 0);
        cout << "::>> User is not registered.Please Create an Account First. \n\n";
    }
}

void create_group_fun(string groupid, string userid, int c_socket)
{
    if (groups.find(groupid) == groups.end())
    {
        Group *newgroup = new Group(groupid, userid);
        groups[groupid] = newgroup;
        char buf[1024] = "true";
        send(c_socket, buf, sizeof(buf), 0);
        cout<<"::>> New Group "<<groupid<<" Created\n";
    }
    else
    {
        char buf[1024] = "false";
        send(c_socket, buf, sizeof(buf), 0);
        cout<<"::>> Group "<<groupid<<" Already Present\n";
    }
}

void join_group_fun(string groupid, string userid, int c_socket)
{
    if (groups.find(groupid) == groups.end())
    {
        char buf[1024] = "false1";
        send(c_socket, buf, sizeof(buf), 0);
        cout<<"::>> Group With Groupid "<<groupid<<" is Not Present\n";
    }
    else
    {
        Group *thatgroup = groups[groupid];
        vector<string> mem = thatgroup->members;
        vector<string> pen_mem = thatgroup->pending_members;

        int flag = 0;

        for (int i = 0; i < mem.size(); i++)
        {
            if (userid == mem[i])
            {
                char buf[1024] = "false2";
                send(c_socket, buf, sizeof(buf), 0);
                cout<<"::>> User "<<userid<<" is already a member of group "<<groupid<<".\n";
                flag = 1;
                return;
            }
        }
        for (int i = 0; i < pen_mem.size(); i++)
        {
            if (userid == pen_mem[i])
            {
                char buf[1024] = "false3";
                send(c_socket, buf, sizeof(buf), 0);
                cout<<"::>> User "<<userid<<" already requested to join group "<<groupid<<".\n";
                flag = 1;
                return;
            }
        }

        if (flag == 0)
        {
            thatgroup->pending_members.push_back(userid);
            char buf[1024] = "true";
            send(c_socket, buf, sizeof(buf), 0);
            cout<<"::>> User "<<userid<<" Requested to join the group "<<groupid<<".\n";
        }
    }
}

void leave_group_fun(string groupid, string userid, int c_socket)
{
    if (groups.find(groupid) == groups.end())
    {
        char buf[1024] = "false1";
        send(c_socket, buf, sizeof(buf), 0);
        cout<<"::>> Group With Groupid "<<groupid<<" is Not Present\n";
    }
    else
    {
        Group *thatgroup = groups[groupid];
        vector<string> mem = thatgroup->members;

        int flag = 0;

        for (int i = 0; i < mem.size(); i++)
        {
            if (userid == mem[i])
            {
                char buf[1024] = "true";
                send(c_socket, buf, sizeof(buf), 0);
                cout<<"::>> User "<<userid<<" left grom the group "<<groupid<<"./n";
                flag = 1;

                auto it = thatgroup->members.begin();
                advance(it, i);
                thatgroup->members.erase(it);

                if (mem[i] == thatgroup->owner)
                {
                    if (thatgroup->members.size() == 0)
                    {
                        groups.erase(groupid);
                    }
                    else
                    {
                        thatgroup->owner = thatgroup->members[0];
                    }
                }

                return;
            }
        }

        if (flag == 0)
        {
            char buf[1024] = "false2";
            send(c_socket, buf, sizeof(buf), 0);
            cout<<"::>> User "<<userid<<" is not a member of group "<<groupid<<"./n";
        }
    }
}

void list_requests_fun(string groupid, string userid, int c_socket)
{
    if (groups.find(groupid) == groups.end())
    {
        char buf[1024] = "false1";
        send(c_socket, buf, sizeof(buf), 0);
         cout<<"::>> Group With Groupid "<<groupid<<" is Not Present\n";
    }

    else
    {
        Group *thatgroup = groups[groupid];
        vector<string> mem = thatgroup->pending_members;

        int flag = 0;

        if (thatgroup->owner != userid)
        {
            char buf[1024] = "false2";
            send(c_socket, buf, sizeof(buf), 0);
            cout<<"::>> User "<<userid<<" is not an admin of the Group "<<groupid<<".\n";
            return;
        }

        char buf[1024] = "true";
        send(c_socket, buf, sizeof(buf), 0);
        cout <<"::>> Sending Pending Requests to user "<<userid<<".\n";

        string memstring;

        for (int i = 0; i < thatgroup->pending_members.size(); i++)
        {
            memstring += thatgroup->pending_members[i] + " ";
        }

        char membuf[5014];
        strcpy(membuf, memstring.c_str());
        send(c_socket, membuf, sizeof(membuf), 0);
    }
}

void accept_request_fun(string groupid, string userid, string ownerid, int c_socket)
{
    if (groups.find(groupid) == groups.end())
    {
        char buf[1024] = "false1";
        send(c_socket, buf, sizeof(buf), 0);
        cout<<"::>> Group With Groupid "<<groupid<<" is Not Present\n";
    }
    else
    {
        Group *thatgroup = groups[groupid];
        vector<string> mem = thatgroup->pending_members;

        int flag = 0;

        if (thatgroup->owner != ownerid)
        {
            char buf[1024] = "false2";
            send(c_socket, buf, sizeof(buf), 0);
            cout<<"::>> User "<<userid<<" is not an admin of the Group "<<groupid<<".\n";
            return;
        }

        for (int i = 0; i < mem.size(); i++)
        {
            if (mem[i] == userid)
            {
                char buf[1024] = "true";
                send(c_socket, buf, sizeof(buf), 0);
                cout<<"::>> "<<ownerid<<" Accepted Request of "<<userid<<" to join the Group "<<groupid<<".\n";
                string memberid = mem[i];

                thatgroup->members.push_back(memberid);

                auto it = thatgroup->pending_members.begin();
                advance(it, i);
                thatgroup->pending_members.erase(it);
                return;
            }
        }

        char buf[1024] = "false3";
        send(c_socket, buf, sizeof(buf), 0);
        cout<<"::>> User "<<userid<<" has not requested to join the Group "<<groupid<<".\n";
    }
}

void list_groups_fun(int c_socket)
{
    if (groups.size() == 0)
    {
        string s = "No_GROUPS_PRESENT";
        char buf[5014];
        strcpy(buf, s.c_str());
        send(c_socket, buf, sizeof(buf), 0);
        cout << "::>> No Groups present in the Network.\n";
        return;
    }
    else
    {
        string s = "";

        for (auto g : groups)
        {
            s += g.first + " ";
        }
        char buf[5014];
        strcpy(buf, s.c_str());
        send(c_socket, buf, sizeof(buf), 0);
        cout << "::>> Group List Sent to the user\n";
    }
}

void list_files_fun(string groupid, string userid, int c_socket)
{
    if (groups.find(groupid) == groups.end())
    {
        char buf[1024] = "false1";
        send(c_socket, buf, sizeof(buf), 0);
        cout<<"::>> Group With Groupid "<<groupid<<" is Not Present\n";
    }
    else
    {
        Group *thatgroup = groups[groupid];
        vector<string> mem = thatgroup->members;

        int flag = 0;

        for (int i = 0; i < mem.size(); i++)
        {
            if (userid == mem[i])
            {
                char buf[1024] = "true";
                send(c_socket, buf, sizeof(buf), 0);
                cout<<"::>> User "<<userid<<" said "<<"List_files.\n";
                flag = 1;
            }
        }

        if (flag == 1)
        {
            string files = "";
            // vector<string>filevector=thatgroup->files;
            if (thatgroup->files.size() == 0)
            {
                cout<<"::>> No files are there in the Group "<<groupid<<".\n";
                files = "NO_FILES_PRESENT";
                char buf[5014];
                strcpy(buf, files.c_str());
                send(c_socket, buf, sizeof(buf), 0);
                return;
            }
            else
            {
                for (auto i : thatgroup->files)
                {
                    files += i.second + "\n";
                }
                char buf[5014];
                strcpy(buf, files.c_str());
                send(c_socket, buf, sizeof(buf), 0);
                return;
            }
        }

        // else
        char buf[1024] = "false2";
        send(c_socket, buf, sizeof(buf), 0);
        cout<<"::>> User "<<userid<<" is not a member of group "<<groupid<<"./n";
    }
}

void upload_file_fun(string filepath, string groupid, string userid, string totalchunks, string peerip, string peerport, int c_socket) //path,totalchunks,peerip,peerport
{

    if (groups.find(groupid) == groups.end())
    {
        cout<<"::>> Group With Groupid "<<groupid<<" is Not Present\n";
        char buf[1024] = "false1";
        send(c_socket, buf, sizeof(buf), 0);
        return;
    }
    else
    {
        Group *thatgroup = groups[groupid];
        vector<string> mem = thatgroup->members;

        int flag = 0;

        for (int i = 0; i < mem.size(); i++)
        {
            if (userid == mem[i])
            {
                flag = 1;
                break;
            }
        }

        if (flag == 0)
        {
            char buf[1024] = "false2";
            send(c_socket, buf, sizeof(buf), 0);
            cout<<"::>> User "<<userid<<" is not a member of group "<<groupid<<"./n";
            return;
        }

        int flag2 = 0;

        if (thatgroup->files.find(filepath) != thatgroup->files.end())
        {
            char buf[1024] = "false3";
            send(c_socket, buf, sizeof(buf), 0);
            cout<<"::>> The File is not present in that Group "<<groupid<<".\n";
            return;
        }

        // Actually uploading now.

        char buf[1024] = "true";
        send(c_socket, buf, sizeof(buf), 0);
        cout << "::>> Uploading New file to the Group "<<groupid<<".\n";

        string base_filename = filepath.substr(filepath.find_last_of("/\\") + 1);
        thatgroup->files[filepath] = base_filename;

        users[userid]->issharable[filepath]=true;  //making that file sharable for that user.

        if (files.find(filepath) != files.end()) //if file is already uploaded to any group just append peer details
        {
            File *thatfile = files[filepath];
            user *Thatuser = users[userid];

            for (int i = 0; i < stoi(totalchunks); i++)
            {
                thatfile->chunkholder[i].push_back(Thatuser);
            }
            thatfile->peertotalchunk[Thatuser] = stoi(totalchunks);

            return;
        }
        else
        {

            File *newfile = new File(filepath, totalchunks);
            newfile->sha1 = getsha1(filepath);

            user *Thatuser = users[userid];

            for (int i = 0; i < stoi(totalchunks); i++)
            {
                newfile->chunkholder[i].push_back(Thatuser);
            }
            newfile->peertotalchunk[Thatuser] = stoi(totalchunks);
            files[filepath] = newfile;
        }

        
    }
}

void sendusingthreads(int k, int c_socket,string filepath,string userid)
{
    File *thatfile = files[filepath];
    char a[1024];

    bzero(a, sizeof(a));
    recv(c_socket, a, sizeof(a), 0);

    string chunktosend1(a);
    int chunktosend = stoi(chunktosend1);

    vector<user *> &peers = thatfile->chunkholder[chunktosend]; //Now i know all peers that have this chunk

    int minchunks = INT_MAX;
    string thatip;
    string thatport;

    for (int i = 0; i < peers.size(); i++) //Peer Selection algo
    {
        if (!peers[i]->isonline or !peers[i]->issharable[filepath])
        {
            continue;
        }
        // if (!peers[i]->isonline)
        // {
        //     continue;
        // }

        user *thatuser = peers[i];
        int chunks = thatfile->peertotalchunk[thatuser];

        if (chunks < minchunks) //Peer Selection
        {
            minchunks = chunks;
            thatip = thatuser->ip;
            thatport = thatuser->port;
        }
    }

    string ipport = thatip + " " + thatport;
    char buf[1024];
    strcpy(buf, ipport.c_str());
    send(c_socket, buf, sizeof(buf), 0); //Sending peerip and port

    bzero(a, sizeof(a));
    recv(c_socket, a, sizeof(a), 0); //Received the chunk no that user downloaded

    string chunkdownloaded(a);
    int chunkdownloadedint = stoi(chunktosend1);

    user *thatuser = users[userid];

    thatfile->chunkholder[chunkdownloadedint].push_back(thatuser);
    thatfile->peertotalchunk[thatuser]++;

    pthread_exit(NULL);
}

void download_file_fun(string groupid, string filename, string dest_path, string userid, int c_socket) //groupid,filename,dest_path
{
    if (groups.find(groupid) == groups.end())
    {
        cout<<"::>> Group With Groupid "<<groupid<<" is Not Present\n";
        char buf[1024] = "false1";
        send(c_socket, buf, sizeof(buf), 0);
        return;
    }
    else
    {
        Group *thatgroup = groups[groupid];
        vector<string> mem = thatgroup->members;

        int flag = 0;

        for (int i = 0; i < mem.size(); i++)
        {
            if (userid == mem[i])
            {
                flag = 1;
                break;
            }
        }

        if (flag == 0)
        {
            char buf[1024] = "false2";
            send(c_socket, buf, sizeof(buf), 0);
            cout<<"::>> User "<<userid<<" is not a member of group "<<groupid<<"./n";
            return;
        }

        int flag2 = 0;

        string filepath;
        for (auto i : thatgroup->files)
        {
            if (i.second == filename)
            {
                flag2 = 1;
                filepath = i.first;
                char buf[1024] = "true";
                send(c_socket, buf, sizeof(buf), 0);
                bzero(buf,sizeof(buf));
                strcpy(buf,filepath.c_str());
                send(c_socket,buf,sizeof(buf),0);
                cout<<"::>> User "<<userid<<" Downloading File "<<filename<<".\n";
                break;
            }
        }

        if (flag2 == 0)
        {
            char buf[1024] = "false3";
            send(c_socket, buf, sizeof(buf), 0);
            cout<<"::>> The File is not present in that Group "<<groupid<<".\n";
            return;
        }

        File *thatfile = files[filepath];
        string totchunks = thatfile->totalchunks;

        users[userid]->issharable[filepath]=true; //making this file sharable.

        char a[1024];
        strcpy(a, totchunks.c_str());
        send(c_socket, a, sizeof(a), 0); //sending total chunks to the peer.

        const char *p=thatfile->sha1.data();

        send(c_socket,p,1024,0);  //sending sha1 of that file.

        for (int i = 0; i < stoi(totchunks); i++)
        {
            thread t(sendusingthreads, i, c_socket,filepath,userid);
            t.join();
        }
    }
}

void logout_fun(string userid, int c_socket)
{
    if (users.find(userid) != users.end())
    {

        user *thatuser = users[userid];
        thatuser->isonline = false;
        char buf[1024] = "true";
        send(c_socket, buf, sizeof(buf), 0);
        cout<<"::>> User "<<userid<<" Logged out.\n";
    }
    else
    {
        char buf[1024] = "false2";
        send(c_socket, buf, sizeof(buf), 0);
        cout<<"User "<<userid<<" Not present.Login Failed.\n";
    }
}

void stop_share_fun(string groupid,string filename,string userid, int c_socket) //gid,filename,uid
{
    if (groups.find(groupid) == groups.end())
    {
        cout<<"::>> Group With Groupid "<<groupid<<" is Not Present\n";
        char buf[1024] = "false1";
        send(c_socket, buf, sizeof(buf), 0);
        return;
    }
    else
    {
        Group *thatgroup = groups[groupid];
        vector<string> mem = thatgroup->members;

        int flag = 0;

        for (int i = 0; i < mem.size(); i++)
        {
            if (userid == mem[i])
            {
                flag = 1;
                break;
            }
        }

        if (flag == 0)
        {
            char buf[1024] = "false2";
            send(c_socket, buf, sizeof(buf), 0);
            cout<<"::>> User "<<userid<<" is not a member of group "<<groupid<<"./n";
            return;
        }

        int flag2 = 0;

        string filepath;
        for (auto i : thatgroup->files)
        {
            if (i.second == filename)
            {
                flag2 = 1;
                filepath = i.first;
                cout<<"::>> User "<<userid<<" Stopped Sharing File "<<filename<<".\n";
                break;
            }
        }

        if (flag2 == 0)
        {
            char buf[1024] = "false3";
            send(c_socket, buf, sizeof(buf), 0);
            cout<<"File "<<filename<<" is Not Present in the Group "<<groupid<<".\n";
            return;
        }

        user *thatuser = users[userid];

        if(thatuser->issharable.find(filepath)==thatuser->issharable.end())
        {
            char buf[1024] = "false4";
            send(c_socket, buf, sizeof(buf), 0);
            return;
        }

        char buf[1024] = "true";
        send(c_socket, buf, sizeof(buf), 0);

        thatuser->issharable[filepath]=false;  //actually stop sharing.
    }
}

void *threadclient(void *sockfd)
{
    int *mysockfdpoint = (int *)sockfd;
    int mysocketfd = *mysockfdpoint;

    while (1)
    {
        cmd.clear();
        char buf[1024];
        recv(mysocketfd, buf, sizeof(buf), 0);
        string s(buf);

        stringstream ss(s);
        string word;

        while (ss >> word)
            cmd.push_back(word);

        if (cmd[0] == "create_user")
        {
            create_user_fun(cmd[1], cmd[2], mysocketfd);
        }
        else if (cmd[0] == "login")
        {
            login_fun(cmd[1], cmd[2], cmd[3], cmd[4], mysocketfd); //userid,pass,peerip,peerport
        }
        else if (cmd[0] == "create_group")
        {
            create_group_fun(cmd[1], cmd[2], mysocketfd); //gid,uid,socket
        }
        else if (cmd[0] == "join_group")
        {
            join_group_fun(cmd[1], cmd[2], mysocketfd); //gid,uid,socket
        }
        else if (cmd[0] == "leave_group")
        {
            leave_group_fun(cmd[1], cmd[2], mysocketfd); //gid,uid,socket
        }
        else if (cmd[0] == "list_requests")
        {
            list_requests_fun(cmd[1], cmd[2], mysocketfd); //gid,uid,socket
        }
        else if (cmd[0] == "accept_request")
        {
            accept_request_fun(cmd[1], cmd[2], cmd[3], mysocketfd); //gid,uid,ownerid,socket
        }
        else if (cmd[0] == "list_groups")
        {
            list_groups_fun(mysocketfd); //socket
        }
        else if (cmd[0] == "list_files")
        {
            list_files_fun(cmd[1], cmd[2], mysocketfd); //gid,uid,socket
        }
        else if (cmd[0] == "upload_file")
        {
            upload_file_fun(cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], mysocketfd); //path,groupid,uid,totalchunks,peerip,peerport
        }
        else if (cmd[0] == "download_file")
        {
            download_file_fun(cmd[1], cmd[2], cmd[3], cmd[4], mysocketfd); //groupid,filename,dest_path,user_id
        }
        else if (cmd[0] == "logout")
        {
            logout_fun(cmd[1], mysocketfd);
        }
        else if (cmd[0] == "stop_share")
        {
            stop_share_fun(cmd[1],cmd[2],cmd[3], mysocketfd);  //gid,filename,uid
        }

        else
        {
            cout << "Enter valid cmd\n";
        }
    }

    pthread_exit(NULL);
}

void mysocket(string ip, string port)
{

    int s_socket;
    s_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (s_socket == -1)
    {
        cout << "Error in connection from Server side.\n";
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(s_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(ip.c_str());
    // inet_aton(ip, &server_address.sin_addr);
    server_address.sin_port = htons(stoi(port));

    if (bind(s_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        cout << "Binding Error" << endl;
        exit(EXIT_FAILURE);
    }

    if (listen(s_socket, 3) < 0)
    {
        cout << "Listening Error\n";
        exit(EXIT_FAILURE);
    }

  

    while (1)
    {
        
        cout <<green<<setw(50)<< "\nListening for peers on ip " << ip << " "<< "and Port " << port <<"..."<<yellow<<endl;

        int client_socket;
        int addrlen;
        if ((client_socket = accept(s_socket, (struct sockaddr *)&server_address, (socklen_t *)&addrlen)) < 0)
        {
            cout << "Accept Error\n";
        }
        else
        {
            cout << "Connection with peer Established.\n" << endl;
        }

        pthread_t tid;
      
        int status = pthread_create(&tid, NULL, threadclient, &client_socket);
        if (status)
        {
            cout << "Error in Creating Thread\n";
        }

    }
 

    close(s_socket);
}

int main(int argc, char const *argv[])
{

    // cout<<"arg0 is "<<argv[0]<<endl;
    // cout<<"arg1 is "<<argv[1]<<endl;
    // cout<<"arg2 is "<<argv[2]<<endl;
    // cout<<"total arg are "<<argc<<endl;

    if (argc != 3)
    {
        perror("mere bhai arguments to shi de (Please Provide the valid arguments thankyou)");
        exit(EXIT_FAILURE);
    }

    string filename = argv[1];
    // int lineno= stoi(argv[2]);

    ifstream infile;
    infile.open(filename);

    string ip;
    infile >> ip;
    // cout << ip << endl;

    string port;
    infile >> port;
    // cout << port << endl;


    system("clear");

    printf("%c[%d;%df", 0x1B, 0, 0); // cursor at 0,0;



    // Intro text

    cout <<green<<R"(
        A     M     M      A     N    N     TTTTT  O O O  RRRR   RRRR   EEEE  N    N  TTTTT
       A A    M M M M     A A    N N  N       T    O   O  R  R   R  R   EE    N N  N    T
      A   A   M  M  M    A   A   N  N N       T    O   O  R R    R R    EEEE  N  N N    T
     A  A  A  M  M  M   A  A  A  N   NN       T    O   O  R  R   R  R   EE    N   NN    T
    A       A M     M  A       A N    N       T    O O O  R   R  R   R  EEEE  N    N    T
    )"<<reset;

    // End of Intro Text


    cout<<"\n\n\n\n\n";
    cout<<red<<"##############################################################################\n\n"<<reset;
    cout<<yellow<<"                         WElCOME TO THE AMAN TORRENT\n";
    cout<<"                             Tracker Strated...\n";



    mysocket(ip, port);

    return 0;
}
