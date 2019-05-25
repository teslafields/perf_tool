import pprint
import sys
import re


if len(sys.argv) < 2:
    print("""
    Usage: python3 parser_fg_perf_data.py <program> <event list (with comma)>\
 <0|1 capture events> <ls events> <mutex> 
    """
    )
    raise SystemExit

class Parser:

    def __init__(self):
        self.app = sys.argv[1]

        self.fg_events_file = 'apps/%s/fine_grain_events' % self.app
        self.block_events_file = 'apps/%s/block_events' % self.app
        self.block_table = 'apps/%s/block_table.csv' % (self.app)
        self.fg_event_table = 'apps/%s/%s_fg_event_table.csv' % (self.app,
                self.app)
        self.perf_file = 'results/fine_grain.data'
        self.__event_table = ''
        self.probes = sys.argv[2].split(',')
        self.probes.append(self.app)
        print(self.probes)

        self.len = len(self.probes)
        self.lsevents = ''
        try:
            lsevents = sys.argv[5]
        except IndexError:
            print("Not collecting counters!")
        else:
            self.lsevents = lsevents.split(',')

        try:
            mutexes = sys.argv[6]
        except IndexError:
            self.MUTEX = False
            print("Not mutex file")
        else:
            self.MUTEX = int(mutexes)

        self.EVENT_TABLE = []
        self.EVENT_DICT = {}
        self.EVENT_LS = {}
        self.timestamps = []
        self.begin = 0

    def split_line_get_time(self, line, timebef=0, count=0):
        splitted = line[1].split('] ')
        time_splitted = splitted[1].split(':')[0].split('.')
        timenow = int(time_splitted[0] + time_splitted[1])
        if timebef:
            timestamp = timenow - timebef
            return timestamp
        return timenow

    def standard(self):
        self.__event_table = self.fg_event_table
        timestamp = 0
        timein = 0
        probe_count = 0
        entry_time = 0
        count = 1
        tmp_list = []
        for i in range(self.len):
            tmp_list.append(self.probes[i])
            self.EVENT_DICT[i] = []
        self.EVENT_TABLE.append(tmp_list.copy())
        with open(self.perf_file, 'r') as f:
            wf = open(self.fg_events_file, 'w')
            for line in f:
                newLine = line.split(' [00')
                if 'probe_%s:in' % self.app in line:
                    entry_time = self.split_line_get_time(newLine)
                elif 'probe_%s:out' % self.app in line:
                    return_time = self.split_line_get_time(newLine, entry_time)
                    tmp_list[-1] = return_time
                    self.EVENT_DICT[probe_count].append(return_time)
                    msg = "{}{}- {}\n\n".format(line, count, return_time)
                    wf.write(msg)
                    self.EVENT_TABLE.append(tmp_list.copy())
                    probe_count = 0
                    count += 1
                else:
                    for probe in self.probes:
                        if probe + 'in' in line and self.begin == 0:
                            wf.write(newLine[1])
                            self.begin = 1
                            timein = self.split_line_get_time(newLine)
                        elif probe + 'out' in line and self.begin == 1:
                            self.begin = 0
                            timestamp = self.split_line_get_time(newLine, timein, count)
                            tmp_list[probe_count] = timestamp
                            self.EVENT_DICT[probe_count].append(timestamp)
                            msg = "{}{}- {}\n".format(line, count, timestamp)
                            wf.write(msg)
                            probe_count += 1
            wf.close()

    def with_mutex(self):
        self.__event_table = self.block_table
        count = 1
        done_times = True

        entry_time = [0 for i in range(self.MUTEX)]
        exit_time = [0 for i in range(self.MUTEX)]
        self.EVENT_TABLE.append(['thread-%d' % i for i in range(self.MUTEX)])

        with open(self.perf_file, 'r') as f:
            wf = open(self.block_events_file, 'w')
            for line in f:
                newLine = line.split(' [00')
                if 'probe_%s:entry' % self.app in line:
                    for i in range(self.MUTEX):
                        if 'p=%d' % i in line:
                            entry_time[i] = self.split_line_get_time(newLine)
                elif 'probe_%s:exit' % self.app in line:
                    for i in range(self.MUTEX):
                        if 'p=%d' % i in line:
                            exit_time[i] = self.split_line_get_time(newLine, entry_time[i])
                        done_times = done_times and exit_time[i] != 0
                    if done_times:
                        self.EVENT_TABLE.append(exit_time.copy())
                        exit_time = [0 for i in range(self.MUTEX)]
                        
                    count += 1
            wf.close()

    def write_csv_table(self):
        with open(self.__event_table, "w") as wf:
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
            msg += "Event {}: Min {}: {} Max {}: {}\n".format(
                self.probes[key],
                self.EVENT_DICT[key].index(list_min)+1,
                list_min,
                self.EVENT_DICT[key].index(list_max)+1,
                list_max
            )

        print("%s" % msg)


if __name__ == "__main__":
    p = Parser()
    if p.MUTEX:
        p.with_mutex()
    else:
        p.standard()
    p.write_csv_table()
    p.print_data()
