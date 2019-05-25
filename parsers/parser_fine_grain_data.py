import pprint
import sys
import re
ARM=0
if len(sys.argv) < 2:
    print("wrong args")
    raise SystemExit

class Parser:

    def __init__(self):
        self.app = sys.argv[1]
        self.perf_file = 'results/fine_grain.data'
        # self.perf_file = 'results/clock_perf_comp.data'
        self.probes = sys.argv[2].split(',')
        print(self.probes)
        self.len = len(self.probes)
        self.lsevents = ''
        try:
            lsevents = sys.argv[5]
        except IndexError:
            print("Not collecting counters!")
        else:
            self.lsevents = lsevents.split(',')
        self.EVENT_TABLE = []
        self.EVENT_DICT = {}
        self.EVENT_LS = {}
        self.timestamps = []
        self.begin = 0

    def split_line_get_time(self, line, timebef=0, count=0):
        global ARM
        splitted = line[1].split('] ')
        time_splitted = splitted[1].split(':')[0].split('.')
        if ARM:
            if timebef:
                time = time_splitted[1][-6:]
                timestamp = int(time_splitted[1]) - int(timebef[1])
                if timestamp < 0:
                    # timestamp = int(time_splitted[0]+time_splitted[1][-6:]) - int(timebef[0]+timebef[1][-6:])
                    timestamp = int(time_splitted[1][-6:]) - int(timebef[1][-6:])
                    print("got neg: {} {} {}".format(time_splitted, timebef, timestamp))
                return timestamp
            return time_splitted
        else:
            timenow = int(time_splitted[0] + time_splitted[1])
        if timebef:
            timestamp = timenow - timebef
            return timestamp
        return timenow
    """
    def split_line_get_time(self, line, timebef=0, count=0):
        splitted = line[1].split('] ')
        time_splitted = splitted[1].split(':')[0].split('.')
        if timebef:
            time = time_splitted[1][-6:]
            timestamp = int(time_splitted[1]) - int(timebef[1])
            if timestamp < 0:
                # timestamp = int(time_splitted[0]+time_splitted[1][-6:]) - int(timebef[0]+timebef[1][-6:])
                timestamp = int(time_splitted[1][-6:]) - int(timebef[1][-6:])
                print("got neg at {}: {} {} {}".format(count, time_splitted, timebef, timestamp))
            elif timestamp/1000000 > 1:
                print("got huge at {}: {} {} {}".format(count, time_splitted, timebef, timestamp))
            return timestamp
        return time_splitted
    """
    def standard(self):
        timestamp = 0
        timein = 0
        capture_ls = 0
        count = 1
        tmp_list = []
        tmp_lsev = []
        for i in range(self.len):
            tmp_list.append(0)
            self.EVENT_DICT[i] = []

        for i in range(len(self.lsevents)):
            tmp_lsev.append(0)
            self.EVENT_LS[i] = []

        with open(self.perf_file, 'r') as f:
            wf = open('apps/%s/fine_grain_events' % self.app, 'w')
            probe_count = 0
            ls_count = 0
            for line in f:
                newLine = line.split(' [00')
                next_line = 0
                for probe in self.probes:
                    if probe + 'in' in line and self.begin == 0:
                        wf.write(newLine[1])
                        self.begin = 1
                        capture_ls = 0
                        timein = self.split_line_get_time(newLine)
                        next_line = 1
                    elif probe + 'out' in line and self.begin == 1:
                        self.begin = 0
                        timestamp = self.split_line_get_time(newLine, timein, count)
                        self.timestamps.append(timestamp)
                        tmp_list[probe_count] = timestamp
                        self.EVENT_DICT[probe_count].append(timestamp)
                        if not self.lsevents:
                            msg = "{}{}- {}\n".format(newLine[1], count, timestamp)
                            wf.write(msg)
                            probe_count += 1
                            if probe_count >= self.len:
                                wf.write('\n')
                                self.EVENT_TABLE.append(tmp_list.copy())
                                probe_count = 0
                                count += 1
                        else:
                            capture_ls = 1
                            wf.write(newLine[1])
                        next_line = 1
                if next_line:
                    continue
                if self.lsevents and capture_ls:
                    capture_ls += 1
                    if capture_ls == len(self.lsevents) + 1:
                        msg = "{}{}- {}\n".format(newLine[1], count, timestamp)
                        wf.write(msg)
                        probe_count += 1
                        if probe_count >= self.len:
                            wf.write('\n')
                            self.EVENT_TABLE.append(tmp_list.copy())
                            probe_count = 0
                            count += 1
                    splitted = newLine[1].split('] ')
                    if splitted:
                        counter_value = re.findall(r'\d+', splitted[1].split(':')[1])
                        if counter_value:
                            counter_value = int(counter_value[0])
                            self.EVENT_LS[ls_count].append(counter_value)
                            ls_count += 1
                            if ls_count >= len(self.lsevents):
                                ls_count = 0
            wf.close()

    def events_capture(self):
        timestamp = 0
        timein = 0

        count = 1
        tmp_list = []
        for i in range(self.len):
            tmp_list.append(0)
            self.EVENT_DICT[i] = []

        with open(self.perf_file, 'r') as f:
            wf = open('apps/%s/fine_grain_events' % self.app, 'w')
            probe_count = 0
            for line in f:
                newLine = line.split(' [00')
                if self.begin == 1:
                    wf.write(newLine[1])
                for probe in self.probes:
                    if probe + 'in' in line and self.begin == 0:
                        self.begin = 1
                        timein = self.split_line_get_time(newLine)
                        wf.write(newLine[1])
                    elif probe + 'out' in line and self.begin == 1:
                        self.begin = 0
                        timestamp = self.split_line_get_time(newLine, timein, count)
                        self.timestamps.append(timestamp)
                        msg = "{}- {}\n".format(count, timestamp)
                        wf.write(msg)
                        tmp_list[probe_count] = timestamp
                        self.EVENT_DICT[probe_count].append(timestamp)
                        probe_count += 1
                        if probe_count >= self.len:
                            wf.write('\n')
                            self.EVENT_TABLE.append(tmp_list.copy())
                            probe_count = 0
                            count += 1
            wf.close()

    def write_csv_table(self):
        with open("results/events_table%s.csv" % sys.argv[4], "w") as wf:
            for item in self.EVENT_TABLE:
                for i in range(len(item)):
                    wf.write("{},".format(item[i]))
                wf.write("\n")

    def read_clock_data(self):
        with open("apps/%s/%s_clock_out" % (self.app, self.app), "r") as rf:
            clock_times = []
            for line in rf:
                clock_times.append(int(line.replace(',', '')))
            msg = ""
            print(self.EVENT_LS)
            for key in self.EVENT_LS:
                list_max = max(self.EVENT_LS[key])
                list_min = min(self.EVENT_LS[key])
                msg += "Count {}: Min {}: {} Max {}: {}\n".format(
                    key,
                    self.EVENT_LS[key].index(list_min)+1,
                    list_min,
                    self.EVENT_LS[key].index(list_max)+1,
                    list_max
                )
            for key in self.EVENT_DICT:
                list_max = max(self.EVENT_DICT[key])
                list_min = min(self.EVENT_DICT[key])
                msg += "Event {}: Min {}: {} Max {}: {}\n".format(
                    key,
                    self.EVENT_DICT[key].index(list_min)+1,
                    list_min,
                    self.EVENT_DICT[key].index(list_max)+1,
                    list_max
                )

            print("%s" % msg)


if __name__ == "__main__":
    p = Parser()
    if int(sys.argv[3]):
        print("Events cap")
        p.events_capture()
    else:
        p.standard()
        print("standard")
    p.write_csv_table()
    p.read_clock_data()
