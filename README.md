to run the tracker :

		g++ tracker.cpp -o tracker -lpthread -lcrypto
		./tracker tracker_info.txt tracker_no
		
to run the client :
		
		g++ client.cpp -o client -lpthread
		./client 127.0.0.2:2020 tracker_info.txt
		
		
on the client i'm using piece selection algo.
on the tracker i'm using peer selection algo.
