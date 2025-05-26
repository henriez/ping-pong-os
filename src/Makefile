CC = gcc
CFLAGS = -Wall

# Common source files
COMMON_SRC = ppos-core-aux.c queue.o ppos-all.o

# Individual source files and their targets
SCHEDULER_SRC = pingpong-scheduler.c
PREEMPCAO_SRC = pingpong-preempcao.c
PREEMPCAO_STRESS_SRC = pingpong-preempcao-stress.c
CONTAB_PRIO_SRC = pingpong-contab-prio.c

# Target executables
SCHEDULER_TARGET = scheduler
PREEMPCAO_TARGET = preempcao
PREEMPCAO_STRESS_TARGET = preempcao-stress
CONTAB_PRIO_TARGET = contab-prio

all: $(SCHEDULER_TARGET) $(PREEMPCAO_TARGET) $(PREEMPCAO_STRESS_TARGET) $(CONTAB_PRIO_TARGET)

$(SCHEDULER_TARGET): $(SCHEDULER_SRC) $(COMMON_SRC)
	gcc -Wall -o $@ $^

$(PREEMPCAO_TARGET): $(PREEMPCAO_SRC) $(COMMON_SRC)
	gcc -Wall -o $@ $^

$(PREEMPCAO_STRESS_TARGET): $(PREEMPCAO_STRESS_SRC) $(COMMON_SRC)
	gcc -Wall -o $@ $^

$(CONTAB_PRIO_TARGET): $(CONTAB_PRIO_SRC) $(COMMON_SRC)
	gcc -Wall -o $@ $^

clean:
	rm -f $(SCHEDULER_TARGET) $(PREEMPCAO_TARGET) $(PREEMPCAO_STRESS_TARGET) $(CONTAB_PRIO_TARGET)

run-scheduler:
	./$(SCHEDULER_TARGET)

run-preempcao:
	./$(PREEMPCAO_TARGET)

run-preempcao-stress:
	./$(PREEMPCAO_STRESS_TARGET)

run-contab-prio:
	./$(CONTAB_PRIO_TARGET)
