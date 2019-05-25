import numpy as np

def get_dataset(csv_file):
    f = open(csv_file)
    data = f.readlines()
    f.close()
    events = data[0].replace(',\n', '')
    events = data[0].replace('\n', '')
    events = events.split(',')
    len_events = len(events)
    all_data = {}
    print(events)
    for i in range(0, len_events):
        all_data.update({
            i: {
                'values': [],
                'name': events[i]
            }
        })

    for key in sorted(all_data.keys()):
        for k in range(1, len(data)):
            line_list = data[k].replace(',\n', '')
            line_list = line_list.split(',')
            value = int(line_list[key])
            all_data[key]['values'].append(value)
        all_data[key]['npvalues'] = np.array(all_data[key]['values'])
    return all_data
