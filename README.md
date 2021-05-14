# ikedamclock for M5Paper

## Usage

1. Install VSCode.
2. Install platformio in VSCode.
3. Open this folder with VSCode.
4. Connect your M5Paper.
5. Create `data/wifi.conf` in the following format:

    ```
    YOUR_WIFI_ESSID<LF>
    YOUR_WIFI_PASSWORD<EOF>
    ```

6. Open the PLATFORMIO pain and run "Platform > Upload Filesystem Image" in PROJECT TASKS.
7. Run "General > Build" in PROJECT TASKS.
8. Run "General > Upload" in PROJECT TASKS.

## Links

* https://docs.m5stack.com/#/en/api/m5paper/system_api
* https://docs.m5stack.com/#/en/api/m5paper/epd_canvas
