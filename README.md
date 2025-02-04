# Project Issue: Encoding-128b-130b-to-5b-6b

## Introduction To Project:
128b/130b encoding is converted to 5b/6b by mapping 5-bit segments to 6-bit codes for DC balance and signal reliability. Error detection is achieved using CRC, which generates a checksum appended to the data. The receiver verifies this checksum to detect transmission errors, ensuring data integrity.

## What Can You Find In This Repository?
There are 2 codes with format of .ino that can be run in arduino ("Encoding.ino" used for transfering data bit by bit and "Encoding-error-detect.ino" is just like pervious code but we added CRC method to detect errors while transfering data), and a PDF file that is a complete document about what we have done.

## Team Members:
1.Mohammadfarhan Bahrami

2.Pouyan Shamsedin

3.Mohammad Shafiezade


