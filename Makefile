LIBS = -lcurl -lcjson
MYSQLCFLAS = -I/usr/include/mysql
MYSQLLIBS = -L/usr/lib/x86_64-linux-gnu -lmysqlclient -lssl -lcrypto -lresolv -lm
webc: webc.o
	${CC} -g -o $@ $< $(LIBS) $(MYSQLCFLAS) $(MYSQLLIBS)

web.o: web.c
	${CC} -g -c $<
