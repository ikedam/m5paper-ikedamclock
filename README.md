# ikedamclock for M5Paper

## Usage

1. Install VSCode.
2. Install platformio in VSCode.
3. Open this folder with VSCode.
4. Connect to your M5Paper.
5. Create `data/wifi.conf` in the following format:

    ```
    YOUR_WIFI_ESSID<LF>
    YOUR_WIFI_PASSWORD<EOF>
    ```

6. Create `data/image.conf` in following format:

    ```
    https://url-to-retrieve-image<LF>
    <EOF>
    ```

    * ikedamclock updates image for every 30 minutes.
    * The URL is expected to return different image files for every access.
    * JPEG and PNG is supported. Progressive JPEG is not supported.

7. (Optional) Put `data/image.jpeg` for the initial image.
8. Open the PLATFORMIO pain and run "Platform > Upload Filesystem Image" in PROJECT TASKS.
9. Run "General > Build" in PROJECT TASKS.
10. Run "General > Upload" in PROJECT TASKS.

## Links

* https://docs.m5stack.com/#/en/api/m5paper/system_api
* https://docs.m5stack.com/#/en/api/m5paper/epd_canvas
