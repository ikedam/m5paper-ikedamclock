# TrueType font subset generator

## Requirements

* Docker
* docker-compose

## Usage

1. Put a font file

    * Example 1: NotoMono-Regular.ttf from https://www.google.com/get/noto/#mono-mono
        * This doesn't contain "Degree Celsius".
    * Example 2: NotoSansMonoCJKjp-Regular.otf from https://www.google.com/get/noto/#sans-jpan

2. Edit `letters.txt` and specify letters you want to contain in the subset font.

3. Run docker-compose:

    ```
    docker-compose run --rm makesubset \
        source-font-file
        letters.txt
        "New font name"
    ```
    
    * Example 1:

        ```
        docker-compose run --rm makesubset \
            NotoMono-Regular.ttf \
            letters.txt \
            "Noto Sans Mono for ikedamclock without dgree celsius"
        ```

    * Example 2:

        ```
        docker-compose run --rm makesubset \
            NotoSansMonoCJKjp-Regular.otf \
            letters.txt \
            "Noto Sans Mono for ikedamclock"
        ```

    * You can ignore following outputs for non true-type source fonts
        (there's no problem as far as I use them):

        ```
        No glyph with unicode U+XXXXX in font
        ```

        ```
        Lookup subtable contains unused glyph Identity.XXX making the whole subtable invalid
        ```

## License

* This software is a part of ikedamclock and is licensed under MIT license. See LICENSE of ikedamclock for more details. This software doesn't provide any restrictions nor warranties to output fonts.
* Output fonts from this software should be treated under the licenses of the source fonts.
