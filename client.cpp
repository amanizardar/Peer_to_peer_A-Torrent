#include <iostream>
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <thread>
#include <time.h>
#include <openssl/sha.h>


// Color codes
const std::string red("\033[0;31m");
const std::string green("\033[1;32m");
const std::string yellow("\033[1;33m");
const std::string cyan("\033[0;36m");
const std::string magenta("\033[0;35m");
const std::string reset("\033[0m");
// End of color codes



using namespace std;

// Variables
vector<string> cmd;
bool isloggedin = false;
string uid;
string peer_server_ip;
string peer_server_port;
string root = "/home/aman/";
std::ofstream fout;
vector<string>groupsjoined;
map<string,pair<string,string>>downloading;  //FileName,filename,groupid
map<string,pair<string,string>>completed;    //Filename,filename,groupid


class File
{
public:
    string name;
    int size;
    int totalchunks;
    // vector<string>chunks;
    map<int, string> chunks;
    // string sha1;
    File(string name1, int size1, int totchunks)
    {
        name = name1;
        size = size1;
        totalchunks = totchunks;
    }
};

map<string, File *> files;


string abspathfun(string source)
{
    string sfp;

    if (source[0] == '/')
    {
        sfp = source;
    }
    else if (source[0] == '~')
    {
        source.erase(0, 1);
        sfp = root + "/" + source;
    }
    else
    {
        if (source[0] == '.')
        {
            source.erase(0, 1);
        }

        char buff[256];
        getcwd(buff, 256);
        string cwd = buff;

        sfp = cwd + "/" + source;
    }

    return sfp;
}

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

    unsigned char result[32];
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

int filesize(string filepath)
{
    ifstream in_file(filepath, ios::binary);
    in_file.seekg(0, ios::end);
    int file_size = in_file.tellg();
    return file_size;
}

void upload_file_fun(string path, string groupid, int c_socket)
{
    string abspath = abspathfun(path);

    // if(files.find(abspath)!=files.end())
    // {
    //     cout<<"This file is already Uploaded\n";
    //     return;
    // }

    ifstream test(abspath);
    if (!test)
    {
        cout << red<<"The file doesn't exist" << endl<<yellow;
        return;
    }

    int size = filesize(abspath);
    int totchunks = ceil(size / (512 * 1024.0));

    // // Debug start
    // cout<<"File size is "<<size<<endl;
    // cout<<"total chunks in the file are "<<totchunks<<endl;
    // // Dubug end

    // Sending details to the tracker
    char a[1024];
    string s = "upload_file " + abspath + " " + groupid + " " + uid + " " + to_string(totchunks) + " " + peer_server_ip + " " + peer_server_port;
    strcpy(a, s.c_str());
    char res[1024];
    send(c_socket, a, sizeof(a), 0);
    recv(c_socket, res, sizeof(res), 0);
    string resstring(res);
    if (resstring == "true")
    {
        cout <<green<< "File Uploaded\n"<<yellow;
    }
    else if (resstring == "false1")
    {
        cout << red<<"Please Enter a Valid Group id\n"<<yellow;
    }
    else if (resstring == "false2")
    {
        cout << cyan<<"You are not a member of this Group\n"<<yellow;
    }
    else if (resstring == "false3")
    {
        cout <<cyan<< "This File is Already Uploaded\n"<<yellow;
    }
    else
    {
        cout <<red<< "Error\n"<<yellow;
    }

    //Uploading file to chunks map
    if (files.find(abspath) != files.end())
    {
        return;
    }

    File *newfile = new File(abspath, size, totchunks);
    files[abspath] = newfile;

    // char buffer[512 * 1024];
    std::ifstream fin(abspath,ios::binary|ios::in);
     std::vector<char> buffer (512*1024,0); //reads only the first 1024 bytes

        size_t total_size = size;
        size_t chunk_size = 512*1024;
  
    size_t total_chunks = ceil(total_size*1.0 / chunk_size);
    size_t last_chunk_size = total_size % chunk_size;

    for (size_t i = 0; i < totchunks; i++)
    {
        size_t this_chunk_size = i == total_chunks - 1 ? last_chunk_size : chunk_size; 

        vector<char> chunk_data(this_chunk_size);
        fin.read(&chunk_data[0],this_chunk_size); 
        string s(chunk_data.begin(),chunk_data.end());
        // cout<<"String size is "<<s.length()<<endl;
        
        // cout<<"vector size "<<chunk_data.size()<<endl;

        string s1=s;
        // cout<<"string s1 size is "<<s1.length()<<endl;
     
        // cout << endl;

        // Old #########################################################################3

        // bzero(buffer, sizeof(buffer));
        // fin.read(buffer, sizeof(buffer));
        // size_t count = fin.gcount();
        // // debug
        // cout << "No. of bytes read = " << count << endl;
        // // end of debug
        // if (!count)
        //     break;

        // string s(buffer);
        newfile->chunks[i] = s;

        // // debug
        // cout<<"string size read is "<<s.length()<<endl;
        // cout<<"Buff size is "<<strlen(buffer)<<endl;

        // End of Debug

    }
    cout<<yellow;
    fin.close();

}

string connect_to_other_peer(string peerip, string peerport,string filepath,int chunkno)
{
    int c_socket = 0;
    if ((c_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cout <<red<< "Error while creating connect peer socket\n"<<yellow;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    // server_address.sin_addr.s_addr=INADDR_ANY;
    server_address.sin_port = htons(stoi(peerport));

    if (inet_pton(AF_INET, peerip.c_str(), &server_address.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }

    int connection_status = connect(c_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (connection_status == -1)
    {
        cout << red<<"Error in connection from client side."<<yellow;
        exit(EXIT_FAILURE);
    }
    // else
    // {
    //     cout << green<<"Congratulations!! Connection Established with the New Peer.\n" << endl<<yellow;
    // }

    string filepath_chunk=filepath+" "+to_string(chunkno);
    char buf[1024];
    strcpy(buf,filepath_chunk.c_str());
    send(c_socket,buf,sizeof(buf),0);  //sending filepath and chunkno which i want to get from the peer.


    char sizechunk[1024];
    recv(c_socket,sizechunk,sizeof(sizechunk),0);
    string sizechu(sizechunk);
    int databytes=stoi(sizechu);

    // // debug
    // cout<<"databytes to be received "<<databytes<<endl;
    // // End debug

    // char chunkdata[512*1024];
    // int count=0;
    // while(count<databytes-1)
    // {
    //     int n = recv(c_socket,chunkdata+count,sizeof(chunkdata)-count,0);
    //     if (n <=0)
    //         break;
    //     count+=n;
    // }
    vector<char>v(databytes);
     int count=0;
    while(count<databytes-1)
    {
        int n = recv(c_socket,&v[count],v.size()-count,0);
        if (n <=0)
            break;
        count+=n;
    }
    // recv(c_socket,&v[0],v.size(),0);

    // cout<<"vector size is "<<v.size()<<endl;

    string s(v.begin(),v.end());

    // cout<<"String len is "<<s.length();


    send(c_socket,buf,sizeof(buf),0); //sending bakbas ack
    
    // int n = recv(c_socket,chunkdata,sizeof(chunkdata),0);
    // string s(chunkdata);

   

    close(c_socket);
    return s;
}


void receiveusingthread(int k,int c_socket,string filepath)
{
    char a[1024];
    bzero(a, sizeof(a));
    strcpy(a, to_string(k).c_str());
    send(c_socket, a, sizeof(a), 0); //asking tracker for the details of peer ip and port. sending chunk number...

    char res[1024];
    recv(c_socket, res, sizeof(res), 0);
    string s(res);
    stringstream ss(s);

    string word;
    vector<string> ipport;

    while (ss >> word)
        ipport.push_back(word);

    string peerip = ipport[0];
    string peerport = ipport[1];

    string chunkdata = connect_to_other_peer(peerip, peerport,filepath,k);

  


    char chunkbuf[512 * 1024];
    strcpy(chunkbuf, chunkdata.c_str());
    // mtx2.lock();
    fout.seekp(k * 512 * 1024, fout.beg);
    // fout << chunkbuf;
    fout<<chunkdata;
    fout.seekp(k * 512 * 1024, fout.beg);
    // mtx2.unlock();

    bzero(a, sizeof(a));
    strcpy(a, to_string(k).c_str());
    send(c_socket, a, sizeof(a), 0); //Telling tracker that i have that chunk now

    pthread_exit(NULL);
    
}

void download_file_fun(string groupid, string filename, string destpath, int c_socket)
{
    // Sending details to the tracker
    char a[1024];
    string s = "download_file " + groupid + " " + filename + " " + destpath + " " + uid;
    strcpy(a, s.c_str());
    char res[1024];
    send(c_socket, a, sizeof(a), 0);
    recv(c_socket, res, sizeof(res), 0);
    string resstring(res);
    string filepath;
    if (resstring == "true")
    {
        char buf[1024];
        recv(c_socket,buf,sizeof(buf),0);
        string filp(buf);
        filepath=filp;

        downloading[filepath]={filename,groupid};  //Adding file to downloading map ...
        cout << green<<"Starting Download.File "<<filename<<" Will be Downloaded Soon.."<<"\n"<<yellow;
    }
    else if (resstring == "false1")
    {
        cout << red<<"Please Enter a Valid Group id\n"<<yellow;
        return;
    }
    else if (resstring == "false2")
    {
        cout <<red<< "You are not a member of this Group\n"<<yellow;
        return;
    }
    else if (resstring == "false3")
    {
        cout << cyan<<"This File is not present in this Group.\n"<<yellow;
        return;
    }
    else
    {
        cout << "Error\n";
        return;
    }
    // i was here 


    bzero(res, sizeof(res));
    recv(c_socket, res, sizeof(res), 0);
    string totchunks(res);

    char sha1[1024];
    recv(c_socket,sha1,sizeof(sha1),0);  //sha 1 of original file received .
    string original_sha1(sha1);


    int totalchunks = stoi(totchunks);
    // // debug
    // cout << "Total Chunks in that file are " << totalchunks << endl;
    // // End debug
    string destabspath = abspathfun(destpath);
    string destcompletepath = destabspath +"/"+ filename;

    fout.open(destcompletepath, ios::out | ios::trunc | ios::binary);

    // Piece Selection algo:
    // Random First
    // Sequential next

    srand(time(0));
    int randomfirst = rand() % totalchunks;

    bzero(a, sizeof(a));
    strcpy(a, to_string(randomfirst).c_str());
    send(c_socket, a, sizeof(a), 0); //asking tracker for the details of peer ip and port. sending chunk number...

    bzero(res,sizeof(res));
    recv(c_socket, res, sizeof(res), 0);
    string s1(res);
    stringstream ss(s1);

    string word;
    vector<string> ipport;

    while (ss >> word)
        ipport.push_back(word);

    string peerip = ipport[0];
    string peerport = ipport[1];

    string chunkdata = connect_to_other_peer(peerip, peerport,filepath,randomfirst);

    // // debug
    //     cout<<"That chunk received from peer is "<<chunkdata.size()<<endl;

    // // Debug

    char chunkbuf[512 * 1024];
    strcpy(chunkbuf, chunkdata.c_str());
    // mtx2.lock();
    fout.seekp(randomfirst * 512 * 1024, fout.beg);
    // fout << chunkbuf;
    fout<<chunkdata;
    fout.seekp(randomfirst * 512 * 1024, fout.beg);
    // mtx2.unlock();

    bzero(a, sizeof(a));
    strcpy(a, to_string(randomfirst).c_str());
    send(c_socket, a, sizeof(a), 0); //Telling tracker that i have that chunk now

    for (int i = 0; i < totalchunks; i++)
    {
        if (i == randomfirst)
            continue;

        thread t(receiveusingthread, i,c_socket,filepath);
        t.join();
    }
    fout.close();

    cout<<"\nFile Downloaded Successfully\n";

    string newsha1=getsha1(destcompletepath);

    if(newsha1==original_sha1)
    {
        cout<<"SHA1 Matched!\n";
    }
    else
    {
        cout<<"SHA1 Mismatched..\n";
    }

    downloading.erase(filepath);
    completed[filepath]={filename,groupid};

}

void *server_functionality(void *sockfd)
{
    int *mysockfdpoint = (int *)sockfd;
    int c_socket = *mysockfdpoint;
    // cout << "Congratulations new peer joined\n";

    char filepath_chunk[1024];
    recv(c_socket,filepath_chunk,sizeof(filepath_chunk),0);

    string s(filepath_chunk);
    stringstream ss(s);
    vector<string>pathchunk;
    string word;
    while(ss>>word)
    pathchunk.push_back(word);

    string thatfilepath=pathchunk[0];
    string thatchunk=pathchunk[1];
    int thatchunkno=stoi(thatchunk);

    File *thatfile=files[thatfilepath];

    string chunkdata=thatfile->chunks[thatchunkno];

    // cout<<"This chunk data size is "<<chunkdata.length()<<endl;

    char sizechunk[1024];
    strcpy(sizechunk,to_string(chunkdata.size()).c_str());
    send(c_socket,sizechunk,sizeof(sizechunk),0);  //sending size of the chunk

    char chunkbuff[512*1024];
    strcpy(chunkbuff,chunkdata.c_str());

    // cout<<"chunkbuff char array size is "<<strlen(chunkbuff)<<endl; 

   const char *p=&chunkdata[0];
    send(c_socket,p,chunkdata.length(),0);  //sending chunkdata to other peer 

    // send(c_socket,chunkbuff,sizeof(chunkbuff),0);  //sending chunkdata to other peer 

    recv(c_socket,filepath_chunk,sizeof(filepath_chunk),0);  //receiving bakbas ack

    pthread_exit(NULL);
}

void peer_as_server()
{
    int s_socket;
    s_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (s_socket == -1)
    {
        cout <<red<< "Error in connection from Server side.\n"<<yellow;
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
    server_address.sin_addr.s_addr = inet_addr(peer_server_ip.c_str());
    // inet_aton(ip, &server_address.sin_addr);
    server_address.sin_port = htons(stoi(peer_server_port));

    if (bind(s_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        cout << "Binding Error" << endl;
        exit(EXIT_FAILURE);
    }

    if (listen(s_socket, 3) < 0)
    {
        cout << "Listening Error";
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "Listening on ip " << peer_server_ip << " "
             << "and Port " << peer_server_port << endl;
    }

    while (1)
    {
        cout << green <<"Listening for other Peers..." << endl<<yellow;

        int client_socket;
        int addrlen;
        if ((client_socket = accept(s_socket, (struct sockaddr *)&server_address, (socklen_t *)&addrlen)) < 0)
        {
            cout << "Accept Error\n";
        }
        // else
        // {
        //     cout << green<<"Connection with peer Established." << endl<<yellow;
        // }

        pthread_t tid;
        // int *client_p=(int*)malloc(sizeof(int));
        // *client_p=client_socket;

        int status = pthread_create(&tid, NULL, server_functionality, &client_socket);
        if (status)
        {
            cout << "Error in Creating Thread\n";
        }

        // send(client_socket, server_msg, sizeof(server_msg), 0);
    }

    close(s_socket);
}

void stop_share_fun(string groupid,string filename,int c_socket)
{
    char a[1024];
    string s = "stop_share " + groupid + " " + filename + " " + uid;
    strcpy(a, s.c_str());
    char res[1024];
    send(c_socket, a, sizeof(a), 0);
    recv(c_socket, res, sizeof(res), 0);
    string resstring(res);
    string filepath;
    if (resstring == "true")
    {

        cout<<green<<"Sharing Stoped for that file.\n"<<yellow;

    }
    else if (resstring == "false1")
    {
        cout << red<<"Please Enter a Valid Group id\n"<<yellow;
        return;
    }
    else if (resstring == "false2")
    {
        cout << cyan<<"You are not a member of this Group\n"<<yellow;
        return;
    }
    else if (resstring == "false3")
    {
        cout << cyan<<"This File is not present in this Group.\n"<<yellow;
        return;
    }
    else if (resstring == "false4")
    {
        cout << cyan<<"User is not already sharing this file.\n"<<yellow;
        return;
    }
    else
    {
        cout << red<<"Error\n"<<yellow;
    }


}

void client_functionality(int c_socket)
{
    while (1)
    {
        cout<<"\n$ ";
        cmd.clear();
        string s;
        getline(cin, s);
        stringstream ss(s);
        string word;

        while (ss >> word)
            cmd.push_back(word);

        if (cmd[0] == "create_user")
        {
            if (cmd.size() != 3)
            {
                cout<<red<<"Sorry Please Enter a Valid command : create_user <Userid> <password>\n"<<yellow;
                continue;
            }
            char a[1024];
            strcpy(a, s.c_str());

            char res[1024];
            send(c_socket, a, sizeof(a), 0);
            recv(c_socket, res, sizeof(res), 0);
            string resstring(res);
            if (resstring == "true")
            {
                uid = cmd[1];
                cout <<green<< "User Account Created.You can login Now.\n"<<yellow;
            }
            else
            {
                cout << cyan<<"User Account already exists.Please Login...\n"<<yellow;
            }
        }

        else if (cmd[0] == "login")
        {
            if (cmd.size() != 3)
            {
                cout<<red<<"Sorry Please Enter a Valid command : login <userid> <password> \n"<<yellow;
                continue;
            }
            if (isloggedin)
            {
                cout << cyan<<"You are already Logged in!\n"<<yellow;
                continue;
            }
            char a[1024];
            s += " " + peer_server_ip + " " + peer_server_port;
            strcpy(a, s.c_str());

            char res[1024];
            send(c_socket, a, sizeof(a), 0);
            recv(c_socket, res, sizeof(res), 0);
            string resstring(res);
            if (resstring == "true")
            {
                isloggedin = true;

                cout << green<<"Login Successful!\n"<<yellow;
            }
            else if (resstring == "false1")
            {
                cout <<red<< "Invalid Password\n"<<yellow;
            }
            else
            {
                cout <<red<< "User Account Does not Exist.Please Try again!\n"<<yellow;
            }

            // login_user_fun();
        }

        else if (cmd[0] == "create_group")
        {
            if (cmd.size() != 2)
            {
                cout<<red<<"Sorry Please Enter a Valid command: create_group <group_id>\n"<<yellow;
                continue;
            }
            if (!isloggedin)
            {
                cout << cyan<<"Please Login First\n"<<yellow;
                continue;
            }
            char a[1024];
            string s1 = s + " " + uid;
            strcpy(a, s1.c_str());

            char res[1024];
            send(c_socket, a, sizeof(a), 0);
            recv(c_socket, res, sizeof(res), 0);
            string resstring(res);
            if (resstring == "true")
            {
                groupsjoined.push_back(cmd[1]);
                cout << green<<"New Group Created Successfully.\n"<<yellow;
            }
            else
            {
                cout <<red<< "Group Creation Failed.\n"<<yellow;
            }
            // Create_group_fun();
        }

        else if (cmd[0] == "join_group")
        {
            if (cmd.size() != 2)
            {
                cout<<red<<"Sorry Please Enter a Valid command: join_group <group_id>\n"<<yellow;
                continue;
            }

            if (!isloggedin)
            {
                cout << cyan<<"Please Login First\n"<<yellow;
                continue;
            }
            char a[1024];
            string s1 = s + " " + uid;
            strcpy(a, s1.c_str());

            char res[1024];
            send(c_socket, a, sizeof(a), 0);
            recv(c_socket, res, sizeof(res), 0);
            string resstring(res);
            if (resstring == "true")
            {
                groupsjoined.push_back(cmd[1]);
                cout <<green<< "Joining Group Resquest Submitted Successfully\n"<<yellow;
            }
            else if (resstring == "false1")
            {
                cout << red<<"Invalid Group.Please Entere a valid Group id.\n"<<yellow;
            }
            else if (resstring == "false2")
            {
                cout << cyan<<"You are Already a member of that Group.\n"<<yellow;
            }
            else
            {
                cout << cyan<<"Request To join the Group already Submitted.Please Wait For The Owner to confirm it.\n"<<yellow;
            }

            // join_group_fun();
        }

        else if (cmd[0] == "leave_group")
        {
            if (cmd.size() != 2)
            {
                cout<<red<<"Sorry Please Enter a Valid command: leave_group <group_id>\n"<<yellow;
                continue;
            }

            if (!isloggedin)
            {
                cout <<cyan<< "Please Login First\n"<<yellow;
                continue;
            }
            char a[1024];
            string s1 = s + " " + uid;
            strcpy(a, s1.c_str());

            char res[1024];
            send(c_socket, a, sizeof(a), 0);
            recv(c_socket, res, sizeof(res), 0);
            string resstring(res);
            if (resstring == "true")
            {
                cout << green<<"You left the Group.\n"<<yellow;
            }
            else if (resstring == "false1")
            {
                cout << red<<"Invalid Group.Please Entere a valid Group id.\n"<<yellow;
            }
            else if (resstring == "false2")
            {
                cout << cyan<<"You are Not a member of that Group.\n"<<yellow;
            }

            // leave_group_fun();
        }

        else if (cmd[0] == "list_requests")
        {
            if (cmd.size() != 2)
            {
                cout<<red<<"Sorry Please Enter a Valid command: list_requests <group_id>\n"<<yellow;
                continue;
            }
            if (!isloggedin)
            {
                cout << cyan <<"Please Login First\n"<<yellow;
                continue;
            }
            char a[1024];
            string s1 = s + " " + uid;
            strcpy(a, s1.c_str());

            char res[1024];
            send(c_socket, a, sizeof(a), 0);
            recv(c_socket, res, sizeof(res), 0);
            string resstring(res);
            if (resstring == "true")
            {
                cout << green<<"All The Pending Members that wants to join group are : ";
                char memres[5014];
                recv(c_socket, memres, sizeof(memres), 0);
                printf("%s \n", memres);
                cout<<yellow;
            }
            else if (resstring == "false1")
            {
                cout << red<<"Invalid Group.Please Entere a valid Group id.\n"<<yellow;
            }
            else if (resstring == "false2")
            {
                cout << cyan<<"You are Not an admin of that Group.\n"<<yellow;
            }

            // request_group_fun();
        }

        else if (cmd[0] == "accept_request")
        {
            if (cmd.size() != 3)
            {
                cout<<red<<"Sorry Please Enter a Valid command\n"<<yellow;
                continue;
            }
            if (!isloggedin)
            {
                cout <<cyan<< "Please Login First\n"<<yellow;
                continue;
            }
            char a[1024];
            string s1 = s + " " + uid;
            strcpy(a, s1.c_str());

            char res[1024];
            send(c_socket, a, sizeof(a), 0);
            recv(c_socket, res, sizeof(res), 0);
            string resstring(res);
            if (resstring == "true")
            {
                cout <<green<< "Accepted !\n"<<yellow;
            }
            else if (resstring == "false1")
            {
                cout <<red<< "Invalid Group.Please Entere a valid Group id.\n"<<yellow;
            }
            else if (resstring == "false2")
            {
                cout <<cyan<< "You are Not an admin of that Group.\n"<<yellow;
            }
            else
            {
                cout << cyan<<"That user is not in request list\n"<<yellow;
            }

            // accept_group_fun();
        }

        else if (cmd[0] == "list_groups")
        {
            if (cmd.size() != 1)
            {
                cout<<red<<"Sorry Please Enter a Valid command\n"<<yellow;
                continue;
            }
            if (!isloggedin)
            {
                cout <<cyan<< "Please Login First\n"<<yellow;
                continue;
            }
            char a[1024];
            string s1 = s;
            strcpy(a, s1.c_str());

            char res[5014];
            send(c_socket, a, sizeof(a), 0);
            recv(c_socket, res, sizeof(res), 0);
            string resstring(res);

            cout << green<<"Groups in the Networks are : " << resstring << endl<<yellow;
            // list_group_fun();
        }

        else if (cmd[0] == "list_files")
        {
            if (cmd.size() != 2)
            {
                cout<<red<<"Sorry Please Enter a Valid command\n"<<yellow;
                continue;
            }
            if (!isloggedin)
            {
                cout << cyan<<"Please Login First\n"<<yellow;
                continue;
            }
            char a[1024];
            string s1 = s + " " + uid;
            strcpy(a, s1.c_str());

            char res[1024];
            send(c_socket, a, sizeof(a), 0);
            recv(c_socket, res, sizeof(res), 0);
            string resstring(res);

            if (resstring == "true")
            {
                cout <<cyan<< "Files Present in that Group : \n"<<green;
                char files[5014];
                recv(c_socket, files, sizeof(files), 0);
                string filestring(files);
                cout << filestring << endl<<yellow;
            }
            else if (resstring == "false1")
            {
                cout <<red<< "Invalid Group.Please Entere a valid Group id.\n"<<yellow;
            }
            else if (resstring == "false2")
            {
                cout << cyan<<"You are Not a member of that Group.\n"<<yellow;
            }

            // list_files_fun();
        }

        else if (cmd[0] == "upload_file")
        {
            if (cmd.size() != 3)
            {
                cout<<red<<"Sorry Please Enter a Valid command\n"<<yellow;
                continue;
            }
            if (!isloggedin)
            {
                cout << cyan<<"Please Login First\n"<<yellow;
                continue;
            }

            thread upthread(upload_file_fun,cmd[1], cmd[2], c_socket);
            upthread.detach();
            cout<<yellow;
            // upload_file_fun(cmd[1], cmd[2], c_socket);
        }

        else if (cmd[0] == "download_file")
        {
            if (cmd.size() != 4)
            {
                cout<<red<<"Sorry Please Enter a Valid command\n"<<yellow;
                continue;
            }
            if (!isloggedin)
            {
                cout << cyan<<"Please Login First\n"<<yellow;
                continue;
            }
            
            thread downthread(download_file_fun,cmd[1], cmd[2], cmd[3], c_socket);
            downthread.detach();
            // download_file_fun(cmd[1], cmd[2], cmd[3], c_socket);

            
        }

        else if (cmd[0] == "logout")
        {
            if (cmd.size() != 1)
            {
                cout<<red<<"Sorry Please Enter a Valid command\n"<<yellow;
                continue;
            }
            if (!isloggedin)
            {
                cout << cyan<<"Please Login First\n"<<yellow;
                continue;
            }
            isloggedin = false;
            // cout<<"Logout Successful\n";

            char a[1024];
            string s1 = s + " " + uid;
            strcpy(a, s1.c_str());

            char res[1024];
            send(c_socket, a, sizeof(a), 0);
            recv(c_socket, res, sizeof(res), 0);
            string resstring(res);
            if (resstring == "true")
            {
                cout << green<<"Logout Successful!\n"<<yellow;
            }
            else
            {
                cout << red<<"User Does not Exist.Login Failed.Please Try again!\n"<<yellow;
            }
            // logout_fun();
        }

        else if (cmd[0] == "show_downloads")
        {
            if (cmd.size() != 1)
            {
                cout<<red<<"Sorry Please Enter a Valid command\n"<<yellow;
                continue;
            }

            if(downloading.size()==0)
            {
                cout<<green<<"[D] : Empty\n"<<yellow;
            }
            else
            {
                for(auto i :downloading)
                {
                    cout<<green<<"[D] "<<"["<<i.second.second<<"] "<<i.second.first<<"\n"<<yellow;
                }
            }
            if(completed.size()==0)
            {
                cout<<green<<"[C] : Empty\n"<<yellow;
            }
            else
            {
                for(auto i :completed)
                {
                    cout<<green<<"[C] "<<"["<<i.second.second<<"] "<<i.second.first<<"\n"<<yellow;
                }
            }


            // show_downloads_fun();
        }

        else if (cmd[0] == "stop_share")
        {
            if (cmd.size() != 3)
            {
                cout<<red<<"Sorry Please Enter a Valid command\n"<<yellow;
                continue;
            }
            if (!isloggedin)
            {
                cout <<cyan<< "Please Login First\n"<<yellow;
                continue;
            }
            

            stop_share_fun(cmd[1],cmd[2],c_socket);
        }

        else
        {
            cout << red<<"Please Enter a valid Command\n"<<yellow;
        }
    }
}

void peer_as_client(string tracker_ip, string tracker_port, string client_ip, string client_port)
{
    int c_socket = 0;
    if ((c_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cout << red<<"Error while creating client socket\n"<<yellow;
    }
    
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    // server_address.sin_addr.s_addr=INADDR_ANY;
    server_address.sin_port = htons(stoi(tracker_port));

    if (inet_pton(AF_INET, tracker_ip.c_str(), &server_address.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }

    int connection_status = connect(c_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (connection_status == -1)
    {
        cout <<red<< "Error in connection from client side.\n"<<yellow;
        exit(EXIT_FAILURE);
    }
    else
    {
        cout <<green<< "Congratulations!! Connection Established with the Tracker." << endl;
        cout<<"Tracker ip is "<<tracker_ip<<" and Port is "<<tracker_port<<".\n"<<yellow;
    }

    client_functionality(c_socket);

    close(c_socket);
}

void clrscr()
{
    system("clear");
}

int main(int argc, char const *argv[])
{

    if (argc != 3)
    {
        perror("mere bhai peer ko arguments to shi de (Please Provide the valid arguments Thankyou)\n");
        exit(EXIT_FAILURE);
    }

    string ipportclient = argv[1];
    vector<string> ipport;
    stringstream ss(ipportclient);
    string item;

    while (getline(ss, item, ':'))
    {
        ipport.push_back(item);
    }

    string client_ip = ipport[0];
    string client_port = ipport[1];
    // cout << "Client ip is " << client_ip << endl;
    // cout << "Client port is " << client_port << endl;
    peer_server_ip = client_ip;
    peer_server_port = client_port;

    thread t1(peer_as_server);

    string filename = argv[2];
    // int lineno= stoi(argv[2]);

    ifstream infile;
    infile.open(filename);

    string tracker_ip;
    infile >> tracker_ip;
    // cout << tracker_ip << endl;

    string tracker_port;
    infile >> tracker_port;
    // cout << tracker_port << endl;

    system("clear");

    printf("%c[%d;%df", 0x1B, 0, 0); // cursor at 0,0;s

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
    cout<<"                             Client Starting...\n\n";



    peer_as_client(tracker_ip, tracker_port, client_ip, client_port);

    atexit(clrscr);

    return 0;
}
