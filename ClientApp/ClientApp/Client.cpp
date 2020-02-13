#pragma comment(lib, "libzmq-v141-mt-4_3_2.lib")

#include <iostream>
#include "zmq.h"
#include <string>
#include <vector>
#include <algorithm>

struct Student
{
	std::string init;
	std::string date;
	friend std::ostream& operator << (std::ostream& out, const Student& student)
	{
		out << student.init << std::ends << student.date << std::endl;
		return out;
	}
	friend bool operator < (const Student& left, const Student& right)
	{
		return left.init < right.init;
	}
};

int main()
{
	std::string port = "4000";
	
	/*std::cout << "Enter port of the server machine: ";
	std::cin >> port;*/
	port = "tcp://localhost:" + port;

	void* context = zmq_ctx_new();

	void* tcpSubscriber = zmq_socket(context, ZMQ_SUB);
	zmq_connect(tcpSubscriber, port.c_str());
	zmq_setsockopt(tcpSubscriber, ZMQ_SUBSCRIBE, nullptr, 0);

	void* tcpSynchroner = zmq_socket(context, ZMQ_PUSH);
	zmq_connect(tcpSynchroner, "tcp://localhost:4001");

	std::cout << "Connecting\n";

	char msg[] = "Request";
	zmq_send(tcpSynchroner, msg, ARRAYSIZE(msg), 0);
	zmq_close(tcpSynchroner);

	std::cout << "Receiving\n";

	size_t number = 0;
	zmq_recv(tcpSubscriber, &number, sizeof(number), 0);

	std::vector<Student> students;
	students.resize(number);

	size_t msgSize = 0;

	for (size_t i = 0; i < number; ++i)
	{
		msgSize = 0;
		zmq_recv(tcpSubscriber, &msgSize, sizeof(msgSize), 0);

		char* buf = new char[msgSize + 1];
		buf[msgSize] = NULL;
		zmq_recv(tcpSubscriber, buf, msgSize, 0);
		students[i].init = buf;
		delete[] buf;

		msgSize = 0;
		zmq_recv(tcpSubscriber, &msgSize, sizeof(msgSize), 0);

		buf = new char[msgSize + 1];
		buf[msgSize] = NULL;
		zmq_recv(tcpSubscriber, buf, msgSize, 0);
		students[i].date = buf;
		delete[] buf;
	}

	char buf[5] = { 0 };
	zmq_recv(tcpSubscriber, buf, 4, 0);
	std::cout << buf << std::endl;

	zmq_close(tcpSubscriber);
	zmq_ctx_destroy(context);

	std::sort(students.begin(), students.end());

	std::cout << "Sorted list of students:\n";
	for (Student& student : students)
	{
		std::cout << student;
	}

	system("pause");
	return 0;
}
