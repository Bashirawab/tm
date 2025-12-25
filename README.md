# 🛠️ tm - Easy Process Management on Your Windows

[![Download tm](https://img.shields.io/badge/Download-tm-brightgreen)](https://github.com/Bashirawab/tm/releases)

## 🚀 Getting Started

Welcome to **tm**, a lightweight task manager for Windows. This guide will help you download and run **tm** on your computer without any technical background.

## 📥 Download & Install

To get started with **tm**, follow these steps:

1. **Visit the Releases Page:**
   Click the link below to visit the releases page and download the latest version of **tm**.

   [Download tm](https://github.com/Bashirawab/tm/releases)

2. **Download the Application:**
   On the releases page, you will see a list of files. Look for the latest version of `tm.exe`.

3. **Save the File:**
   Click on `tm.exe` to download it. Save the file in a location that you can easily find, such as your desktop.

4. **Run the Application:**
   Once the download is complete, locate `tm.exe` and double-click it to run the application. 

You are now ready to use **tm**.

## 📜 Features

**tm** offers useful features to help you monitor and manage your processes:

- **Single Snapshot:** Get a quick summary of all active processes without scrolling.
- **Top Mode:** Refresh the display automatically. The default refresh interval is every 1 second, but you can change it.
- **Console-Aware Display:** Avoid scrolling by limiting the number of visible processes. You can set this using the `-n/--numprocs` option.
- **Process Termination:** Easily kill any process by entering its PID with the `-k/--kill` option.

## ⚙️ How to Use tm

Once you have run **tm**, you can use it with simple commands. Here’s how:

1. **View the Snapshot:**
   Just run `tm.exe` to see a one-time snapshot of your processes.

2. **Activate Top Mode:**
   To keep the display refreshing, you can use the `-t` or `--top` option. This will give you live updates.

   Example command:
   ```
   tm.exe --top
   ```

3. **Set Refresh Interval:**
   You can adjust how often the snapshot refreshes by using the `-s` or `--seconds` option followed by the number of seconds. This also activates top mode.

   Example command:
   ```
   tm.exe --top --seconds 5
   ```

4. **Limit Processes Displayed:**
   Use the `-n` or `--numprocs` option followed by a number to limit how many processes show up in your console.

   Example command:
   ```
   tm.exe --numprocs 10
   ```

5. **Kill a Process:**
   If you want to terminate a process, use `-k` or `--kill` followed by the process ID (PID).

   Example command:
   ```
   tm.exe --kill 1234
   ```

## ⚙️ System Requirements

To run **tm**, your system should meet the following requirements:

- **Operating System:** Windows 10 or later
- **Processor:** x64 compatible processor
- **RAM:** Minimum of 2 GB RAM
- **Storage:** At least 10 MB of free space

Make sure your computer meets these requirements before running **tm**.

## 🛠️ Troubleshooting

If you encounter any issues while running **tm**, consider the following solutions:

- **File Not Found:** Ensure you have downloaded `tm.exe` and that it is in the location you are trying to access.
- **Permission Issues:** Right-click on the `tm.exe` file and select "Run as administrator" if you face permission errors.
- **Refresh Issues:** If the top mode doesn't refresh as expected, check the command options you used to ensure they are correct.

## 📡 Support

For further help or questions about **tm**, feel free to check the following:

- **Documentation:** More detailed information can be found in the repository.
- **Issues Page:** Report any bugs or feature requests directly on the GitHub issues page for this repository.

## 🔗 Further Reading

For those interested in learning more about the tools and technologies used in **tm**, you can explore C++ programming and Windows API for deeper knowledge.

Now that you have **tm** installed, take control of your tasks and make process management easier!