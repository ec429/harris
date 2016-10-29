# Makefile for harris game stats extractors

STATS_OBJS := stats/hashtable.o hist_record.o
STATS_INCLUDES := $(STATS_OBJS:.o=.h)

stats: stats/read_history stats/como stats/cshr stats/kills stats/fighters stats/raids stats/bombers

stats/%: stats/%.c $(OBJS) $(STATS_OBJS) $(INCLUDES) $(STATS_INCLUDES)
	$(CC) $(CFLAGS) $(CPPFLAGS) -I . $(SDLFLAGS) $< $(OBJS) $(STATS_OBJS) -o $@ $(LDFLAGS) $(LIBS) $(SDL)

