import pprint
import sys
import re


if len(sys.argv) < 2:
    print("""
    Usage: python3 parser_mutex.py <path> <program> <threads> 
    """
    )
    raise SystemExit

class Parser:

    def __init__(self):
        self.path = sys.argv[1]
        self.app = sys.argv[2]

        self.block_events_file = '%s/block_events' % self.path
        self.block_table = '%s/block_table.csv' % self.path

        self.perf_file = 'perf_text.data' % self.path

        self.THREADS = int(sys.argv[3])
        self.i_reference = 0

        self.EVENT_DICT = {}
        self.EVENT_TABLE = []

    def split_line_get_time(self, line, timebef=0, count=0):
        splitted = line[1].split('] ')
        time_splitted = splitted[1].split(':')[0].split('.')
        timenow = int(time_splitted[0] + time_splitted[1])
        if timebef:
            timestamp = timenow - timebef
            return timestamp
        return timenow

    def parse_data(self):
        count = 0

        entry_time = [0 for i in range(self.THREADS)]
        exit_time = [0 for i in range(self.THREADS)]
        self.EVENT_TABLE.append(['thread-%d' % i for i in range(self.THREADS)])
        self.EVENT_DICT = {key: [] for key in range(self.THREADS)}
        fw = open(self.block_events_file, 'w+')
        with open(self.perf_file, 'r') as f:
            for line in f:
                fw.write("{}- {}".format(count, line))
                newLine = line.split(' [00')
                if 'probe_%s:entry' % self.app in line:
                    for i in range(self.THREADS):
                        if 'p=%d' % i in line:
                            entry_time[i] = self.split_line_get_time(newLine)
                elif 'probe_%s:exit' % self.app in line:
                    for i in range(self.THREADS):
                        if 'p=%d' % i in line:
                            exit_time[i] = self.split_line_get_time(newLine, entry_time[i])
                            self.EVENT_DICT[i].append(exit_time[i])
                            if i == self.i_reference:
                                count += 1

                    cond = [x for x in exit_time if x == 0]
                    if not cond:
                        exit_time = [0 for i in range(self.THREADS)]
        fw.close()

    def write_csv_table(self):
        total_len = len(self.EVENT_DICT[0])
        for k in range(total_len):
            l = [self.EVENT_DICT[m][k] for m in range(self.THREADS)]
            self.EVENT_TABLE.append(l)
        with open(self.block_table, "w") as wf:
            for item in self.EVENT_TABLE:
                for i in range(len(item)-1):
                    wf.write("{},".format(item[i]))
                wf.write("{}".format(item[-1]))
                wf.write("\n")

    def print_data(self):
        msg = ''
        for key in self.EVENT_DICT:
            list_max = max(self.EVENT_DICT[key])
            list_min = min(self.EVENT_DICT[key])
            msg += "Thread {}: Min {}: {} Max {}: {}\n".format(
                key,
                self.EVENT_DICT[key].index(list_min)+1,
                list_min,
                self.EVENT_DICT[key].index(list_max)+1,
                list_max
            )
            sort_times = sorted(self.EVENT_DICT[key])
            print('median: %d' % sort_times[int(len(sort_times)/2)])
            # media = sum(sort_times)/len(sort_times)
            # for item in sort_times:
            #     if item > media*1.5:
            #         arr.append(item)
            print("outliers: %d %s" % (len(sort_times[-20:]), sort_times[-20:]))

        print("%s" % msg)


if __name__ == "__main__":
    p = Parser()
    p.parse_data()
    p.write_csv_table()
    p.print_data()
