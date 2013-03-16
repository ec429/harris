#!/usr/bin/python
"""value - total value tracking"""

import sys
import hhist, hdata

class BadTimeSeries(Exception): pass
class UnpairedEntry(BadTimeSeries): pass

# TODO: need ability to parse savefiles (esp. INIT)
# Intention for this report is to compute (total value of all bombers + cash) for each day

if __name__ == '__main__':
	entries = hhist.import_from_save(sys.stdin)
