# SRC = $(wildcard *.cc) $(wildcard ../lib/*.c)
# LIBS = ../lib/tcp_buffer.o ../lib/tcp_utils.o ../lib/thpool.o
# CC = g++
# CFLAGS += -I../include -Wall -Wextra -fsanitize=address -lpthread -g

# all: FS FC

# FS: FS.cc $(LIBS)
# 	$(CC) $(CFLAGS) $^ -o $@

# FC: FC.cc $(LIBS)
# 	$(CC) $(CFLAGS) $^ -o $@

# include ${SRC:.c=.d}
# include ${SRC:.cc=.d}

# %.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# %.o: %.cc
# 	$(CC) $(CFLAGS) -c $< -o $@

# %.d: %.c
# 	@set -e; rm -f $@; \
# 	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
# 	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
# 	rm -f $@.$$$$

# %.d: %.cc
# 	@set -e; rm -f $@; \
# 	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
# 	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
# 	rm -f $@.$$$$

# clean:
# 	rm -f *.o *.d FS FC
# .PHONY: all clean



# # 定义编译器
# CC=g++

# # 定义编译选项
# CFLAGS += -I../include -Wall -Wextra -fsanitize=address -lpthread -g
# # LDFLAGS=-Llib -lyourlib

# # 定义目标文件
# TARGET=FS FC

# # 定义源文件
# SOURCES=FS.cc 

# # 从源文件生成目标文件的依赖
# OBJECTS=$(SOURCES:.cc=.o)
# OBJECTS+=$(wildcard ../lib/*.o)

# # 从.d文件生成依赖
# DEPENDENCIES=$(SOURCES:.cc=.d)
# DEPENDENCIES+=$(wildcard ../lib/*.d)

# # 默认目标
# all: $(TARGET)

# # 规则：从.cc和.c文件生成.o文件
# %.o: %.cc
# 	$(CC) $(CFLAGS) -c $< -o $@

# %.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# # 规则：链接目标文件生成可执行文件
# $(TARGET): $(OBJECTS)
# 	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $@

# # 包含依赖文件
# -include $(DEPENDENCIES)

# # 清理编译生成的文件
# clean:
# 	rm -f $(OBJECTS) $(TARGET) $(DEPENDENCIES)

# # 声明伪目标
# .PHONY: all clean


all: FS FC

FS: FS.c ../lib/tcp_buffer.o ../lib/tcp_utils.o ../lib/thpool.o
	gcc -I../include -Wall -Wextra -fsanitize=address -lpthread -g $^ -o $@

FC: FC.c ../lib/tcp_buffer.o ../lib/tcp_utils.o ../lib/thpool.o
	gcc -I../include -Wall -Wextra -fsanitize=address -lpthread -g $^ -o $@

clean:
	rm -f *.o FS FC
.PHONY: all clean
