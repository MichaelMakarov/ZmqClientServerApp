#pragma comment(lib, "libzmq-v141-mt-4_3_2.lib")

#include <iostream>
#include "zmq.h"
#include <string>
#include <fstream>
#include <set>
#include <future>

     
struct Student
{
	std::string init;
	std::string date;
	friend std::ostream& operator << (std::ostream & out, const Student & student)
	{
		out << student.init << std::ends << student.date << std::endl;
		return out;
	}
	friend bool operator < (const Student& left, const Student& right)
	{
		return left.init < right.init;
	}
};

const char* firstFileName = "student_file_1.txt";
const char* secondFileName = "student_file_2.txt";
std::set<Student> gStudents;
std::string gTCPpubPort = "4000";
std::string gTCPsyncPort = "4001";
std::string gHTTPsendPort = "4002";


bool IsDigit(char c);

bool ReadTxtFile(const char*, std::set<Student>&);
void PrintDataSet();

void RunTCPMessaging();
void ReplyTCPMsg(size_t*, void*);
void WaitTCPSubs(size_t*, void*);


int main()
{
	// reading files
	if (ReadTxtFile(firstFileName, gStudents))
		std::cout << "First file was read.\n";
	else return 1;
	if (ReadTxtFile(secondFileName, gStudents))
		std::cout << "Second file was read.\n";
	else return 1;

	RunTCPMessaging();
	

	return 0;
}


void RunTCPMessaging()
{
	void* context = zmq_ctx_new();

	std::string addrPub = "tcp://*:" + gTCPpubPort;
	void* tcpPublisher = zmq_socket(context, ZMQ_PUB);
	zmq_bind(tcpPublisher, addrPub.c_str());

	std::string addrSync = "tcp://*:" + gTCPsyncPort;
	void* tcpSynchroner = zmq_socket(context, ZMQ_PULL);
	zmq_bind(tcpSynchroner, addrSync.c_str());

	size_t subsCount = 0;
	auto wait = std::async(std::launch::async, WaitTCPSubs, &subsCount, tcpSynchroner);
	while (true)
	{
		ReplyTCPMsg(&subsCount, tcpPublisher);
	}
	
	zmq_close(tcpPublisher);
	zmq_close(tcpSynchroner);
	zmq_ctx_destroy(context);
}

void WaitTCPSubs(size_t* number, void* tcpSocket)
{
	std::cout << "Waiting for subscribers\n";
	while (true)
	{
		char buf[] = "Request";
		int res = zmq_recv(tcpSocket, buf, ARRAYSIZE(buf), 0);
		if (res != -1)
		{
			(*number)++;
			std::cout << "New sub has connected.\n";
		}
	}

}

void ReplyTCPMsg(size_t* number, void* publisher)
{
	if ((*number) > 0)
	{
		std::cout << "Sending...\n";
		size_t msgSize = gStudents.size();
		zmq_send(publisher, &msgSize, sizeof(msgSize), ZMQ_SNDMORE);

		for (auto& student : gStudents)
		{
			msgSize = student.init.size();
			zmq_send(publisher, &msgSize, sizeof(msgSize), ZMQ_SNDMORE);

			msgSize = student.init.size();
			char* buf = new char[msgSize + 1];
			buf[msgSize] = NULL;
			memcpy_s(buf, msgSize, student.init.c_str(), msgSize);
			zmq_send(publisher, buf, msgSize, ZMQ_SNDMORE);
			delete[] buf;

			msgSize = student.date.size();
			zmq_send(publisher, &msgSize, sizeof(msgSize), ZMQ_SNDMORE);

			msgSize = student.date.size();
			buf = new char[msgSize + 1];
			buf[msgSize] = NULL;
			memcpy_s(buf, msgSize, student.date.c_str(), msgSize);
			zmq_send(publisher, buf, msgSize, ZMQ_SNDMORE);
			delete[] buf;
		}
		zmq_send(publisher, "Done", 4, 0);
		(*number)--;
	}
}

void PrintDataSet()
{
	std::cout << "Info:\n";
	for (auto& student : gStudents)
	{
		std::cout << student << std::endl;
	}
}

bool ReadTxtFile(
	const char* fileName,
	std::set<Student>& students)
{
	std::ifstream fin;
	fin.open(fileName);

	if (fin.is_open())
	{
		char buf = NULL;

		while (!fin.eof())
		{
			fin.read(&buf, sizeof(buf));
			if (IsDigit(buf))
			{
				fin.seekg(-1, fin.cur);

				Student student;
				int id;

				fin >> id;
				fin.read(&buf, sizeof(buf));
				fin.read(&buf, sizeof(buf));

				while (!IsDigit(buf))
				{
					student.init.push_back(buf);
					fin.read(&buf, sizeof(buf));
					if (fin.eof()) break;
				}
				student.init.pop_back();

				while (buf != '\n')
				{
					student.date.push_back(buf);
					fin.read(&buf, sizeof(buf));
					if (fin.eof()) break;
				}

				students.insert(student);
			}
			else if (buf == '-')
			{
				char line[MAX_PATH] = { 0 };
				fin.getline(line, MAX_PATH - 1, '\n');
			}
		}
		fin.close();
	}
	else {
		std::cout << "Failed to open file " << fileName << std::endl <<
			" Please shure that it is situated in the project folder.\n";
		return false;
	}
	return true;
}

bool IsDigit(char c)
{
	return (
		c == '0' ||
		c == '1' ||
		c == '2' ||
		c == '3' ||
		c == '4' ||
		c == '5' ||
		c == '6' ||
		c == '7' ||
		c == '8' ||
		c == '9');
}

