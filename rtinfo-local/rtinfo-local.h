#ifndef __RTINFO_LOCAL_H
	#define __RTINFO_LOCAL_H
	
	#define UPDATE_INTERVAL		1000000
	
	#define RATE_COLD	(A_BOLD | COLOR_PAIR(5))
	#define RATE_ACTIVE	(A_BOLD | COLOR_PAIR(8))
	#define RATE_LOW	(A_BOLD | COLOR_PAIR(6))
	#define RATE_MIDDLE	(A_BOLD | COLOR_PAIR(3))
	#define RATE_HIGH	(A_BOLD | COLOR_PAIR(4))

	#define LEVEL_COLD	(A_BOLD | COLOR_PAIR(5))
	#define LEVEL_ACTIVE	(COLOR_PAIR(1))
	#define LEVEL_WARN	(A_BOLD | COLOR_PAIR(3))
	#define LEVEL_HIGH	(A_BOLD | COLOR_PAIR(4))

	#define TITLE_STYLE	(A_BOLD | COLOR_PAIR(6))

	int x, y;
	char *units[] = {"Ko", "Mo", "Go", "To"};
	char *uptime_units[] = {"min", "hrs", "days"};

	/* Network rate colors */
	int rate_limit[] = {
			2   * 1024,		/* 2   Ko/s | Magenta	*/
			100 * 1024,		/* 100 Ko/s | Cyan	*/
			1.5 * 1024 * 1024,	/* 1.5 Mo/s | Yellow	*/
			20  * 1024 * 1024,	/* 20  Mo/s | Red	*/
	};

	/* Memory (RAM/SWAP) level colors */
	int memory_limit[] = {
			30,	/* 30% | White  */
			50,	/* 50% | Yellow */
			85,	/* 85% | Red    */
	};

	int cpu_limit[] = {
			30,	/* 30% | White  */
			50,	/* 50% | Yellow */
			85,	/* 85% | Red	*/
	};
	
	int localside();
#endif
