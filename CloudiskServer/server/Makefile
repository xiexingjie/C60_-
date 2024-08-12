# 获取当前目录下的所有 .c 文件
Srcs := $(wildcard *.c)
# 添加 ../mysql_tools 目录下的 .c 文件
Srcs += $(wildcard ../mysql_tools/*.c)
# 将 .c 文件名替换为 .o 文件名
Outs := $(patsubst %.c, %.o, $(Srcs))

# MySQL 配置标志
SH_CFLAGS = $(shell mysql_config --cflags)
SH_LIBS = $(shell mysql_config --libs)

# 可执行文件名
BIN := CloudiskServer
# 编译器及编译标志
CC := gcc
CFLAGS = -w -g -I../mysql_tools -lssl -lcrypto -lcrypt # 添加 -I../mysql_tools

# 默认目标
ALL: $(BIN)

$(BIN): $(Outs)
	$(CC) -o $(BIN) $^ $(CFLAGS) $(SH_CFLAGS) $(SH_LIBS)

# 编译每个 .c 文件生成对应的 .o 文件
# 使用自动变量 $< 和 $@ 分别表示输入文件和输出文件
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS) $(SH_CFLAGS)

# 处理特定目录下的 .o 文件生成规则
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS) $(SH_CFLAGS)

.PHONY: clean rebuild ALL

clean:
	$(RM) $(Outs) $(BIN)

# 重建目标
rebuild: clean ALL
