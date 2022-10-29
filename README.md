# PEER TO PEER FILE SHARING SYSTEM - MINI TORRENT
Developed a P2P multimedia file sharing system using TCP that allows uploading and downloading files from multiple available seeders, like Bit Torrent.  
Users can share and download files from the group they belong. All the files were split into chunks.  
At every peer, a multi-threaded environment enables functionalities like multiple files parallel uploading from one peer and parallel downloading of different chunks of one file from multiple peers. It provides support for fault tolerance and parallel downloading. Technologies used: C++, Pthread, Socket Programming.,SHA-1 Hashing

- Implemented user verification by login authentication module.
- Each user is a part of one or many groups.
- List of the available groups can be fetched.
- User can request to join any group or create his/her own group, and hence becoming the owner of that group.
- Group owner can accept/reject the request of any user to join the group.
- User is allowed to leave the groups.
- Upload the file across the groups : by doing so all the member of that group can list the available files in that group that can be downloaded.
- Divided the file into logical “pieces” , wherein the size of each piece is 512KB.
- SHA1 Hashing : Suppose the file size is 1024KB, then divided it into two pieces of 512KB each and SHA1 hash of each part is calculated, assume that the hashes are HASH1 & HASH2 then the corresponding hash string would be H1H2 , where H1 & H2 are starting 20 characters of HASH1 & HASH2 respectively and hence H1H2 is 40 characters.
- while downloading a file, all the peers having the chunks of that file are retrieved using the tracker to know which peer is having what chunk of that file after that downloading of multiple pieces(chunks) are done parallely from multiple peers.
- Which piece to be downloaded first and which next is decided by the piece selection algorithm.
- If multiple peers are having the same chunk of the file then from which peer that chunk should be downloaded is decided by peer selection algorithm.
- As soon as a piece of file gets downloaded it will be available for sharing.
- Error handling is done via matching of downloaded file SHA-1 hash to the original file SHA-1 hash and if mismatched then corresponding chunk is again asked for downloading.
- Client side uses piece selection algo : Random first and then rarest remaining piece selection algorithm.
- Tracker side implements peer selection algo : Selects the peer which is having minimun number of chunks of that particular file because from the peer having more number of chunk of that file, other peers may also be requesting some pieces of the file so reduce the load on that peer it is better to ask from the one having less number of chunks of that file.
- Other functionalities : Show downloads, stop download, Stop sharing the files, Logout,


### Commands : 

to run the tracker :

		g++ tracker.cpp -o tracker -lpthread -lcrypto
		./tracker tracker_info.txt tracker_no
		
to run the client :
		
		g++ client.cpp -o client -lpthread
		./client 127.0.0.2:2020 tracker_info.txt
		
Create User Account : 

		create_user <user_id> <passwd>
Login: 

  		login <user_id> <passwd>
Create Group: 
		
		create_group <group_id>
Join Group: 

		join_group <group_id>
Leave Group: 

		leave_group <group_id>
List pending join: 
	
		requests list_requests <group_id>
Accept Group Joining Request: 

		accept_request <group_id> <user_id>
List All Group In Network: 

		list_groups
List All sharable Files In Group: 

		list_files <group_id>
		
Upload File: 

		upload_file <file_path> <group_id>
Download File: 

		download_file <group_id> <file_name> <destination_path>
Logout: 

		logout
Show_downloads: 
	
		show_downloads
Stop sharing: 

		stop_share <group_id> <file_name>
		
		

