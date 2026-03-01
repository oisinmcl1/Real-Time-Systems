"""
Parsing Collected NTP Logs

Oisín Mc Laughlin - 22441106
"""
from collections import defaultdict
from datetime import datetime
import re
import os
import csv
import statistics
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

INPUT_FILE = "ntp-logs/ntplog.txt"
OUTPUT_DIR = "parsed-ntp-logs"

SERVER_NAMES = {
    '140.203.204.77': 'Ireland',
    'ntp0.cam.ac.uk': 'UK',
    'ptbtime1.ptb.de': 'Germany',
    'time-a-g.nist.g': 'US',
    'ntp1.tuxfamily.org': 'France',
    'ns1.anu.edu.au': 'Australia',
    'ntp-b3.nict.go.': 'Japan'
}


def parse_ntp_log(input_file):
    """
    Parses the NTP log file and extracts relevant data points.
    :param input_file: Path to NTP log file
    :return: List of dictionaries containing parsed data points
    """
    data_rows = []
    current_timestamp = None

    with open(input_file, 'r') as f:
        for line in f:
            line = line.strip()

            # Check for timestamp line
            timestamp_match = re.match(r'=== (.+?) ===', line) # Match lines like "=== 2024-06-01 12:00:00 UTC ==="
            if timestamp_match:
                timestamp_str = timestamp_match.group(1) # Match the timestamp part
                try:
                    current_timestamp = datetime.strptime(timestamp_str, '%Y-%m-%d %H:%M:%S %Z') # Try parsing with timezone
                except ValueError:
                    print("Error failed to parse timestamp")
                    break

            # Skip header lines and empty lines
            if not line or line.startswith('remote') or line.startswith('===') or line.startswith('='):
                continue

            # Split the line into parts based on whitespace
            parts = line.split()

            # Ensure we have enough parts to parse (at least 9 fields)
            if len(parts) < 9:
                continue

            # Extract the server status symbol if present
            remote = parts[0]
            status = ''
            if remote and remote[0] in ['*', '+', '-', ' ', 'x', 'o', '#']:
                status = remote[0]
                remote = remote[1:]

            # Map the remote server to a known name if possible
            refid = parts[1]

            # Skip lines with .STEP., .INIT., or other error indicators
            if refid in ['.STEP.', '.INIT.', '.RATE.']:
                continue

            try:
                st = int(parts[2])
                t = parts[3]
                when = parts[4]
                poll = int(parts[5])
                reach = int(parts[6])
                delay = float(parts[7])
                offset = float(parts[8])
                jitter = float(parts[9])

                # Convert 'when' field
                when_seconds = 0
                if when != '-':
                    if 'm' in when:
                        when_seconds = int(when.replace('m', '')) * 60
                    elif 'h' in when:
                        when_seconds = int(when.replace('h', '')) * 3600
                    else:
                        try:
                            when_seconds = int(when)
                        except ValueError:
                            when_seconds = 0

                data_rows.append({
                    'timestamp': current_timestamp.strftime('%Y-%m-%d %H:%M:%S'),
                    'timestamp_obj': current_timestamp,
                    'status': status,
                    'remote': remote,
                    'refid': refid,
                    'stratum': st,
                    'type': t,
                    'when_seconds': when_seconds,
                    'poll': poll,
                    'reach': reach,
                    'delay_ms': delay,
                    'offset_ms': offset,
                    'jitter_ms': jitter
                })
            except (ValueError, IndexError) as e:
                print(f"Warning: Skipping malformed line: {line}")
                continue

    print(f"Successfully parsed {len(data_rows)} data points")
    return data_rows


def write_main_csv(data_rows, output_dir):
    """
    Writes the parsed NTP data to a main CSV file.
    :param data_rows: List of dictionaries containing parsed data points
    :param output_dir: Directory to save the output CSV file
    :return: Path to the created CSV file
    """

    output_file = os.path.join(output_dir, "ntp_data_all.csv")

    fieldnames = ['timestamp', 'status', 'remote', 'refid', 'stratum', 'type',
                  'when_seconds', 'poll', 'reach', 'delay_ms', 'offset_ms', 'jitter_ms']

    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        for row in data_rows:
            row_copy = row.copy()
            row_copy.pop('timestamp_obj', None)
            writer.writerows([row_copy])

    print(f"Created: {output_file}")
    return output_file


def write_combined_csv(data_rows, output_dir):
    """
    Writes a combined CSV file that includes the server location based on the remote server IP or hostname.
    :param data_rows: List of dictionaries containing parsed data points
    :param output_dir: Directory to save the output CSV file
    :return: Path to the created CSV file
    """

    output_file = os.path.join(output_dir, "ntp_data_with_locations.csv")

    with open(output_file, 'w', newline='') as f:
        fieldnames = ['timestamp', 'location', 'server', 'delay_ms', 'offset_ms', 'jitter_ms']
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()

        for row in data_rows:
            server = row['remote']
            location = SERVER_NAMES.get(server, server)
            writer.writerow({
                'timestamp': row['timestamp'],
                'location': location,
                'server': server,
                'delay_ms': row['delay_ms'],
                'offset_ms': row['offset_ms'],
                'jitter_ms': row['jitter_ms']
            })

    print(f"Created: {output_file}")
    return output_file


def write_per_server_files(data_rows, output_dir):
    """
    Writes separate CSV files for each server, containing only the relevant data points for that server.
    :param data_rows: List of dictionaries containing parsed data points
    :param output_dir: Directory to save the output CSV files
    :return: List of paths to the created CSV files
    """

    servers_data = defaultdict(list) # Group data by server

    for row in data_rows:
        server = row['remote']
        servers_data[server].append(row)

    files_created = []

    for server, data in servers_data.items():
        location_name = SERVER_NAMES.get(server, server.replace('.', '_'))
        filename = os.path.join(output_dir, f"{location_name}.csv")

        with open(filename, 'w', newline='') as f:
            fieldnames = ['timestamp', 'delay_ms', 'offset_ms', 'jitter_ms']
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()

            for row in data:
                writer.writerow({
                    'timestamp': row['timestamp'],
                    'delay_ms': row['delay_ms'],
                    'offset_ms': row['offset_ms'],
                    'jitter_ms': row['jitter_ms']
                })

        files_created.append(filename)
        print(f"Created: {filename}")

    return files_created


def calculate_statistics(data_rows):
    """
    Calculates statistics (min, max, mean, stddev) for delay, offset, and jitter for each server.
    :param data_rows: List of dictionaries containing parsed data points
    :return: List of dictionaries containing statistics for each server
    """

    servers_data = defaultdict(lambda: {'delay': [], 'offset': [], 'jitter': []})

    for row in data_rows:
        server = row['remote']
        servers_data[server]['delay'].append(row['delay_ms'])
        servers_data[server]['offset'].append(row['offset_ms'])
        servers_data[server]['jitter'].append(row['jitter_ms'])

    stats = []

    print("\n" + "=" * 80)
    print("STATISTICS BY SERVER")
    print("=" * 80)

    for server in sorted(servers_data.keys()):
        data = servers_data[server]
        location = SERVER_NAMES.get(server, server)

        delay_stats = {
            'min': min(data['delay']),
            'max': max(data['delay']),
            'mean': statistics.mean(data['delay']),
            'stdev': statistics.stdev(data['delay']) if len(data['delay']) > 1 else 0
        }

        offset_stats = {
            'min': min(data['offset']),
            'max': max(data['offset']),
            'mean': statistics.mean(data['offset']),
            'stdev': statistics.stdev(data['offset']) if len(data['offset']) > 1 else 0
        }

        jitter_stats = {
            'min': min(data['jitter']),
            'max': max(data['jitter']),
            'mean': statistics.mean(data['jitter']),
            'stdev': statistics.stdev(data['jitter']) if len(data['jitter']) > 1 else 0
        }

        print(f"\n{location} ({server})")
        print("-" * 40)
        print(f"Data points: {len(data['delay'])}")
        print("\nDelay (ms):")
        print(f"  Min:    {delay_stats['min']:.4f}")
        print(f"  Max:    {delay_stats['max']:.4f}")
        print(f"  Mean:   {delay_stats['mean']:.4f}")
        print(f"  StdDev: {delay_stats['stdev']:.4f}")
        print("\nJitter (ms):")
        print(f"  Min:    {jitter_stats['min']:.4f}")
        print(f"  Max:    {jitter_stats['max']:.4f}")
        print(f"  Mean:   {jitter_stats['mean']:.4f}")
        print(f"  StdDev: {jitter_stats['stdev']:.4f}")

        stats.append({
            'Server': location,
            'IP_Hostname': server,
            'Data_Points': len(data['delay']),
            'Delay_Min': delay_stats['min'],
            'Delay_Max': delay_stats['max'],
            'Delay_Mean': delay_stats['mean'],
            'Delay_StdDev': delay_stats['stdev'],
            'Offset_Min': offset_stats['min'],
            'Offset_Max': offset_stats['max'],
            'Offset_Mean': offset_stats['mean'],
            'Offset_StdDev': offset_stats['stdev'],
            'Jitter_Min': jitter_stats['min'],
            'Jitter_Max': jitter_stats['max'],
            'Jitter_Mean': jitter_stats['mean'],
            'Jitter_StdDev': jitter_stats['stdev']
        })

    return stats


def write_statistics_summary(stats, output_dir):
    """
    Writes a summary CSV file containing the calculated statistics for each server.
    :param stats: List of dictionaries containing statistics for each server
    :param output_dir: Directory to save the output CSV file
    :return: None
    """

    output_file = os.path.join(output_dir, "statistics_summary.csv")

    with open(output_file, 'w', newline='') as f:
        fieldnames = ['Server', 'IP_Hostname', 'Data_Points',
                      'Delay_Min', 'Delay_Max', 'Delay_Mean', 'Delay_StdDev',
                      'Offset_Min', 'Offset_Max', 'Offset_Mean', 'Offset_StdDev',
                      'Jitter_Min', 'Jitter_Max', 'Jitter_Mean', 'Jitter_StdDev']
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(stats)

    print(f"\nCreated statistics summary: {output_file}")


def create_plots(data_rows, output_dir):
    """
    Creates line plots for delay and jitter over time for each server.
    :param data_rows: List of dictionaries containing parsed data points
    :param output_dir: Directory to save the output plot images
    :return: None
    """

    # Group data by server
    servers = defaultdict(lambda: {'time': [], 'delay': [], 'jitter': []})
    for row in data_rows:
        servers[row['remote']]['time'].append(row['timestamp_obj'])
        servers[row['remote']]['delay'].append(row['delay_ms'])
        servers[row['remote']]['jitter'].append(row['jitter_ms'])

    # Plot delay
    plt.figure(figsize=(12, 6))
    for server, data in servers.items():
        # filter out any entries without a valid timestamp
        times = []
        values = []
        for t, v in zip(data['time'], data['delay']):
            if t is None:
                continue
            times.append(t)
            values.append(v)
        if not times:
            continue
        plt.plot(times, values, marker='o', label=SERVER_NAMES.get(server, server))
    plt.xlabel('Time (UTC)')
    plt.ylabel('Delay (ms)')
    plt.title('NTP Server Delay')
    plt.legend()
    plt.grid(True)

    ax = plt.gca()
    locator = mdates.AutoDateLocator()
    try:
        formatter = mdates.ConciseDateFormatter(locator)
    except AttributeError:
        formatter = mdates.DateFormatter('%Y-%m-%d %H:%M')
    ax.xaxis.set_major_locator(locator)
    ax.xaxis.set_major_formatter(formatter)

    fig = plt.gcf()
    fig.autofmt_xdate()
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'delay_plot.png'))
    plt.show()

    # Plot jitter
    plt.figure(figsize=(12, 6))
    for server, data in servers.items():
        times = []
        values = []
        for t, v in zip(data['time'], data['jitter']):
            if t is None:
                continue
            times.append(t)
            values.append(v)
        if not times:
            continue
        plt.plot(times, values, label=SERVER_NAMES.get(server, server), alpha=0.5)
    plt.xlabel('Time (UTC)')
    plt.ylabel('Jitter (ms)')
    plt.title('NTP Server Jitter')
    plt.legend()
    plt.grid(True)

    # Apply the same date formatting for the jitter plot
    ax = plt.gca()
    ax.xaxis.set_major_locator(locator)
    ax.xaxis.set_major_formatter(formatter)
    fig = plt.gcf()
    fig.autofmt_xdate()
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'jitter_plot.png'))
    plt.show()

    print(f"Created: {os.path.join(output_dir, 'delay_plot.png')}")
    print(f"Created: {os.path.join(output_dir, 'jitter_plot.png')}")


def main():
    data_rows = parse_ntp_log(INPUT_FILE)
    write_main_csv(data_rows, OUTPUT_DIR)
    write_combined_csv(data_rows, OUTPUT_DIR)
    write_per_server_files(data_rows, OUTPUT_DIR)

    stats = calculate_statistics(data_rows)
    write_statistics_summary(stats, OUTPUT_DIR)

    create_plots(data_rows, OUTPUT_DIR)


if __name__ == "__main__":
    main()
