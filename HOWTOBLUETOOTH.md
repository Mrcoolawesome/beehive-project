# Wii Balance Board Bluetooth Setup

**1. Open the Bluetooth control utility in your terminal:**
```bash
bluetoothctl
```

2. Execute the commands in order using this specific MAC address:

```bash
trust 00:1F:32:22:03:BF
pair 00:1F:32:22:03:BF
connect 00:1F:32:22:03:BF
```


3. Exit the utility once connected:
```bash
exit
```