CXX ?= g++

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2

endif

server: main.cpp  ./timer/lst_timer.cpp ./http_conn/http_conn1.cpp ./log/log.cpp ./CGImysql/sql_connection_pool.cpp  webserver.cpp config.cpp
	$(CXX) -g -o server  $^ $(CXXFLAGS) -lpthread -lmysqlclient

clean:
	rm  -r server
