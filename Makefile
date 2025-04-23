CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
LIBS = -lcurl -ljansson

all: ddfetch

ddfetch: main.c
	$(CC) $(CFLAGS) -o ddfetch main.c $(LIBS)

report:
	@echo "📊 Generating Emissions Report"
	@awk -f awk/total.awk emissions.txt
	@awk -f awk/total_usd.awk emissions.txt
	@awk -f awk/max.awk emissions.txt
	@awk -f awk/max_usd.awk emissions.txt
	@awk -f awk/average.awk emissions.txt
	@awk -f awk/average_usd.awk emissions.txt

metrics:
	@echo "--- Total USD Value ---"
	@awk -f awk/total.awk emissions.txt
	@awk -f awk/total_usd.awk emissions.txt
	@echo ""
	@echo "--- Max USD Value ---"
	@awk -f awk/max_usd.awk emissions.txt
	@echo ""
	@echo "--- Average USD Value ---"
	@awk -f awk/average_usd.awk emissions.txt
	@echo ""
	@echo "--- Daily Deltas ---"
	@awk -f awk/daily_deltas.awk emissions.txt
	@echo ""
	@echo "--- ROI (auto-inferred initial) ---"
	@awk -f awk/roi.awk emissions.txt
	@echo ""
	@echo "--- Price Volatility ---"
	@awk -f awk/volatility.awk emissions.txt
	@echo ""

roi:
	@echo "Usage: make roi initial=3200"
	@awk -v user_initial=$(initial) -f awk/roi.awk emissions.txt
	
clean:
	rm -f ddfetch
	rm -f emissions.txt
