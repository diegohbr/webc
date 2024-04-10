LIBS = -lcurl
MYSQLCFLAS = -I/usr/include/mysql
MYSQLLIBS = -L/usr/lib/x86_64-linux-gnu -lmysqlclient -lssl -lcrypto -lresolv -lm
CJSON = cJSON/cJSON.c
webc: webc.o
	${CC} -g -o $@ $< $(LIBS) $(CJSON) $(MYSQLCFLAS) $(MYSQLLIBS)

web.o: web.c
	${CC} -g -c $<
