import re
import sys
import pprint
import getopt
from datetime import datetime

def exit_now(what):
    print('Error: %s' % what, file=sys.stderr)
    sys.exit()

def usage():
    print('Usage: python3 %s [options] -b program_bin' % sys.argv[0])
    print('Options are:')
    print('  -e    event list separeted by comma')
    print('  -m    probes name list within a function separeted by comma')
    print('  -t    threads number')
    print('  --ls  leader sampling list separated by comma')

def get_args():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "b:e:m:t:v:", ["help", "ls="])
    except getopt.GetoptError as err:
        exit_now(err)
    print(opts)
    d = dict()
    for o, a in opts:
        if o == '--help':
            usage()
        elif o == '--ls':
            d['lsevents'] = a.split(',')
        elif o == '-m':
            d['mevents'] = a.split('.')
        elif o == '-e':
            d['events'] = a.split('.')
        elif o == '-t':
            d['threads'] = int(a)
        elif o == '-b':
            d['bin'] = a
    if not d.get('bin'):
        exit_now('program_bin not provided')
    return d


class Parser:
    delimiter = '-'*100
    perf_data_file = 'perf_text.data'

    def __init__(self, bin, events=[], mevents=[], lsevents=[], threads=[]):
        self.bin = bin
        self.events = events
        self.mevents = mevents
        self.lsevents = lsevents
        self.threads = threads
        self.e_flag = True if events else False
        self.m_flag = True if mevents else False
        self.ls_flag = True if lsevents else False
        self.timestamp_arr = []

    def split_line_get_time(self, line, timebef=0):
        splitted = line[1].split('] ')
        time_splitted = splitted[1].split(':')[0].split('.')
        timenow = int(time_splitted[0] + time_splitted[1])
        if timebef:
            return timenow - timebef
        return timenow

    def leader_sampling(self):
        app_list = self.APPS.get(app)
        if not app_list: print("Not collecting events")
        tmp_list = []
        for i in range(len(app_list)+1):
            tmp_list.append(0)
        for item in app_list:
            tmp_list[item['index']] = item['name']
        global timeBef
        with open("perf_text.data") as f:
            with open("%s/%s_events" % (path, app), "w") as wf:
                self.timestamp_arr = 0
                count = 0
                for line in f:
                    newLine = line.split(' [00')
                    if self.begin and 'probe_%s:out' % app in line:
                        timeStamp = self.split_line_get_time(newLine, timeBef)
                        if count > 2 and timeStamp < 0:
                            print("%s | neg: %s" % (count, timeStamp))
                        elif count > 2 and self.too_big(timeStamp):
                            print("%s | big: %s" %  (count, timeStamp))
                        self.timestamp_arr.append(timeStamp)
                        wf.write(newLine[1])
                        if self.num_lsevents != 0:
                            self.capture_ls = 1
                        else:
                            buf = "{}- Time elapsed: {}\n{}\n".format(count, timeStamp, delimiter)
                            wf.write(buf)
                            count+=1
                        tmp_list[-1] = timeStamp
                    elif 'probe_%s:in' % app in line:
                        self.begin = 1
                        timeBef = self.split_line_get_time(newLine)
                        buf = "{}- Time elapsed: {}\n{}\n{}".format(count, timeStamp, "-"*100, newLine[1])
                        wf.write(buf)
                        self.EVENT_TABLE.append(tmp_list.copy())
                        self.capture_ls = 0
                        count+=1
                    elif self.capture_ls > 0 and self.capture_ls <= self.num_lsevents:
                        splitted = newLine[1].split('] ')
                        if splitted:
                            counter_value = re.findall(r'\d+', splitted[1].split(':')[1])
                            if counter_value:
                                counter_value = counter_value[0]
                                for item in app_list:
                                    if item['name'] in newLine[1]:
                                        tmp_list[item['index']] = counter_value
                                        wf.write(newLine[1])
                                        self.capture_ls += 1
                self.EVENT_TABLE.append(tmp_list.copy())

    def parse_e_data(self):
        entries = len(self.events)+1
        csv_table, ev_header_arr, ev_count_arr = [], [], [0]*entries
        for ev in self.events:
            ev_header_arr.append(ev)
        ev_header_arr.append('Time')
        csv_table.append(ev_header_arr.copy())
        with open(self.perf_data_file) as f:
            with open("results/%s_events" % self.bin, "w") as wf:
                timestamp, cpu, count, capture_events  = 0, 0, 1, False
                for line in f:
                    # line = line.replace('        ', '')
                    splitted_line = line.split(' [0')
                    if len(splitted_line) < 2:
                        continue
                    if 'probe_%s:out' % self.bin in line:
                        timestamp = self.split_line_get_time(splitted_line, timebef)
                        ev_count_arr[-1] = timestamp
                        self.timestamp_arr.append(timestamp)
                        wf.write(line)
                        buf = "{}- Elapsed time: {}\n{}\n".format(count, timestamp, self.delimiter)
                        wf.write(buf)
                        count+=1
                        csv_table.append(ev_count_arr.copy())
                        capture_events, ev_count_arr = False, [0]*entries
                    elif 'probe_%s:in' % self.bin in line:
                        timebef = self.split_line_get_time(splitted_line)
                        wf.write(line)
                        capture_events = True
                    elif capture_events:
                        wf.write(line)
                        for ev in ev_header_arr:
                            if ev.replace('*', '') in splitted_line[1]:
                                ev_count_arr[ev_header_arr.index(ev)] += 1
        print(csv_table)
        return csv_table

    def write_csv_table(self, csv_table):
        if not csv_table:
            return
        ts = '' #datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
        with open("results/csv/%s_%s.csv" % (ts, self.bin), "w") as wf:
            for ev_arr in csv_table:
                for i in range(len(ev_arr)-1):
                    wf.write("%s," % ev_arr[i])
                wf.write("%s\n" % ev_arr[-1])

    def print_time(self, csv_table):
        if not csv_table:
            print("Error: Events and probes not collected, perf script empty?")
            return
        print('Dataset info:')
        print(self.delimiter)
        print("Sample Min %s: %s us Max %s: %s us" % (
                self.timestamp_arr.index(min(self.timestamp_arr))+1,
                min(self.timestamp_arr),
                self.timestamp_arr.index(max(self.timestamp_arr))+1,
                max(self.timestamp_arr)
            ))
        arr = []
        sort_times = sorted(self.timestamp_arr)
        print('median: %d' % sort_times[int(len(sort_times)/2)])
        media = sum(sort_times)/len(sort_times)
        for item in sort_times:
            if item > 1000: #media*1.5:
                arr.append(item)
        print("outliers: %d %s" % (len(arr), arr))

    def parse_data(self):
        if self.ls_flag and self.m_flag:
            self.parse_mls_data()
        elif self.e_flag and self.m_flag:
            self.parse_mls_data()
        elif self.m_flag:
            self.parse_m_data()
        elif self.ls_flag:
            self.parse_ls_data()
        #elif self.e_flag:
        else:
            csv_table = self.parse_e_data()
        self.write_csv_table(csv_table)
        self.print_time(csv_table)


if __name__ == "__main__":
    args = get_args()
    p = Parser(**args)
    p.parse_data()
