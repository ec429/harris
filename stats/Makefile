# Makefile for harris game stats extractors

STATS_OBJS := stats/hashtable.o hist_record.o
STATS_INCLUDES := $(STATS_OBJS:.o=.h)

stats: stats/read_history stats/como stats/cshr stats/kills stats/fighters stats/raids

stats/%: stats/%.c $(OBJS) $(STATS_OBJS) $(INCLUDES) $(STATS_INCLUDES)
	$(CC) $(CFLAGS) $(CPPFLAGS) -I . $(SDLFLAGS) $(LDFLAGS) $(LIBS) $< $(OBJS) $(STATS_OBJS) -o $@ $(SDL)

