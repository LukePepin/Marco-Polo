hider@ISE-hider:~/Documents/Marco-Polo $ python system_verification.py
============================================================
            MARCO POLO: SYSTEM VERIFICATION TOOL            
============================================================
Scanning for connected Arduino boards...
Found 2 serial port(s). Querying status...

🔍 Testing /dev/ttyS0 (n/a)...
   [1/5] Opening serial port...
   [2/5] Waiting for connection to stabilize...
   [3/5] Clearing input/output buffers...
   [4/5] Sending GET_STATUS command...
   [5/5] Reading response...
   (No initial response. Retrying GET_STATUS query...)
   ❌ ERROR: No response to GET_STATUS query. Timed out.
      Ensure the Arduino has the correct sketch uploaded and is running.
   Closing serial port...
------------------------------------------------------------
🔍 Testing /dev/ttyACM0 (Nano 33 BLE)...
   [1/5] Opening serial port...
   [2/5] Waiting for connection to stabilize...
   [3/5] Clearing input/output buffers...
   [4/5] Sending GET_STATUS command...
   [5/5] Reading response...
   ↳ Response: "STATUS: OK (Hider), ID: DECA - model: 1, version: 3, revision: 0"
   ✅ SUCCESS: Node is online and responding.
   Closing serial port...
------------------------------------------------------------

============================================================
   VERIFICATION SUMMARY
============================================================
Total serial ports scanned: 2
Fully functional UWB nodes verified: 1
⚠️  Some nodes failed verification. Please review the errors above.
============================================================
hider@ISE-hider:~/Documents/Marco-Polo $ sudo usermod -a -G dialout $USER
[sudo] password for hider: 
hider@ISE-hider:~/Documents/Marco-Polo $ python hider_node.py
========================================
        HIDER NODE (UWB TX)             
========================================
✅ Connected to Arduino on /dev/ttyACM0

>>> Instructions: Press ENTER to trigger a wireless PING broadcast.
>>> Press Ctrl+C to exit.


>>> Sending 'SEND_PING' Command to Arduino...

[Arduino]: PING_SENT_SUCCESSFULLY

>>> Sending 'SEND_PING' Command to Arduino...

>>> Sending 'SEND_PING' Command to Arduino...


^C
Exiting Hider...

[Error reading serial]: [Errno 9] Bad file descriptor
^C^C^C^C^C^C^C^C^C^C^C^[[A^[[B^CTraceback (most recent call last):
  File "/home/hider/Documents/Marco-Polo/hider_node.py", line 71, in <module>
    main()
    ~~~~^^
  File "/home/hider/Documents/Marco-Polo/hider_node.py", line 68, in main
    arduino.close()
    ~~~~~~~~~~~~~^^
  File "/usr/lib/python3/dist-packages/serial/serialposix.py", line 533, in close
    os.close(self.fd)
    ~~~~~~~~^^^^^^^^^
KeyboardInterrupt
Exception ignored in: Serial<id=0x7f9e4afc10, open=True>(port='/dev/ttyACM0', baudrate=115200, bytesize=8, parity='N', stopbits=1, timeout=1, xonxoff=False, rtscts=False, dsrdtr=False)
Traceback (most recent call last):
  File "/usr/lib/python3/dist-packages/serial/serialposix.py", line 533, in close
    os.close(self.fd)
OSError: [Errno 9] Bad file descriptor
^C
hider@ISE-hider:~/Documents/Marco-Polo $ ^C
hider@ISE-hider:~/Documents/Marco-Polo $ python hider_node.py
========================================
        HIDER NODE (UWB TX)             
========================================
✅ Connected to Arduino on /dev/ttyACM0

>>> Instructions: Press ENTER to trigger a wireless PING broadcast.
>>> Press Ctrl+C to exit.


>>> Sending 'SEND_PING' Command to Arduino...
^C
Exiting Hider...

[Error reading serial]: [Errno 9] Bad file descriptor
^C^C^C^C^C^C^CTraceback (most recent call last):
  File "/home/hider/Documents/Marco-Polo/hider_node.py", line 71, in <module>
    main()
    ~~~~^^
  File "/home/hider/Documents/Marco-Polo/hider_node.py", line 68, in main
    arduino.close()
    ~~~~~~~~~~~~~^^
  File "/usr/lib/python3/dist-packages/serial/serialposix.py", line 533, in close
    os.close(self.fd)
    ~~~~~~~~^^^^^^^^^
KeyboardInterrupt
Exception ignored in: Serial<id=0x7fba9cfc10, open=True>(port='/dev/ttyACM0', baudrate=115200, bytesize=8, parity='N', stopbits=1, timeout=1, xonxoff=False, rtscts=False, dsrdtr=False)
Traceback (most recent call last):
  File "/usr/lib/python3/dist-packages/serial/serialposix.py", line 533, in close
    os.close(self.fd)
OSError: [Errno 9] Bad file descriptor

hider@ISE-hider:~/Documents/Marco-Polo $ python system_verification.py
============================================================
            MARCO POLO: SYSTEM VERIFICATION TOOL            
============================================================
Scanning for connected Arduino boards...
Found 2 serial port(s). Querying status...

🔍 Testing /dev/ttyS0 (n/a)...
   [1/5] Opening serial port...
   [2/5] Waiting for connection to stabilize...
   [3/5] Clearing input/output buffers...
   [4/5] Sending GET_STATUS command...
   [5/5] Reading response...
   (No initial response. Retrying GET_STATUS query...)
   ❌ ERROR: No response to GET_STATUS query. Timed out.
      Ensure the Arduino has the correct sketch uploaded and is running.
   Closing serial port...
------------------------------------------------------------
🔍 Testing /dev/ttyACM0 (Nano 33 BLE)...
   [1/5] Opening serial port...
   [2/5] Waiting for connection to stabilize...
   [3/5] Clearing input/output buffers...
   [4/5] Sending GET_STATUS command...
   ❌ ERROR: Could not communicate with port: Write timeout
------------------------------------------------------------

============================================================
   VERIFICATION SUMMARY
============================================================
Total serial ports scanned: 2
Fully functional UWB nodes verified: 0
⚠️  Some nodes failed verification. Please review the errors above.
============================================================
^C^C^CException ignored in: Serial<id=0x7f848d5090, open=True>(port='/dev/ttyACM0', baudrate=115200, bytesize=8, parity='N', stopbits=1, timeout=1.5, xonxoff=False, rtscts=False, dsrdtr=False)
Traceback (most recent call last):
  File "/usr/lib/python3/dist-packages/serial/serialposix.py", line 533, in close
    os.close(self.fd)
KeyboardInterrupt: 
hider@ISE-hider:~/Documents/Marco-Polo $ python hider_node.py
========================================
        HIDER NODE (UWB TX)             
========================================
✅ Connected to Arduino on /dev/ttyACM0

>>> Instructions: Press ENTER to trigger a wireless PING broadcast.
>>> Press Ctrl+C to exit.


>>> Sending 'SEND_PING' Command to Arduino...
really slow here about to unplug
>>> Sending 'SEND_PING' Command to Arduino...

[Error reading serial]: [Errno 5] Input/output error
^C
Exiting Hider...
hider@ISE-hider:~/Documents/Marco-Polo $ replugged
bash: replugged: command not found
hider@ISE-hider:~/Documents/Marco-Polo $ python hider_node.py
========================================
        HIDER NODE (UWB TX)             
========================================
✅ Connected to Arduino on /dev/ttyACM0

>>> Instructions: Press ENTER to trigger a wireless PING broadcast.
>>> Press Ctrl+C to exit.


>>> Sending 'SEND_PING' Command to Arduino...

[Arduino]: PING_SENT_SUCCESSFULLY
^C
Exiting Hider...

[Error reading serial]: [Errno 9] Bad file descriptor
^C^C^C^CTraceback (most recent call last):
  File "/home/hider/Documents/Marco-Polo/hider_node.py", line 71, in <module>
    main()
    ~~~~^^
  File "/home/hider/Documents/Marco-Polo/hider_node.py", line 68, in main
    arduino.close()
    ~~~~~~~~~~~~~^^
  File "/usr/lib/python3/dist-packages/serial/serialposix.py", line 533, in close
    os.close(self.fd)
    ~~~~~~~~^^^^^^^^^
KeyboardInterrupt
Exception ignored in: Serial<id=0x7f9fdafc10, open=True>(port='/dev/ttyACM0', baudrate=115200, bytesize=8, parity='N', stopbits=1, timeout=1, xonxoff=False, rtscts=False, dsrdtr=False)
Traceback (most recent call last):
  File "/usr/lib/python3/dist-packages/serial/serialposix.py", line 533, in close
    os.close(self.fd)
OSError: [Errno 9] Bad file descriptor

hider@ISE-hider:~/Documents/Marco-Polo $ python system_verification.py
============================================================
            MARCO POLO: SYSTEM VERIFICATION TOOL            
============================================================
Scanning for connected Arduino boards...
Found 2 serial port(s). Querying status...

🔍 Testing /dev/ttyS0 (n/a)...
   [1/5] Opening serial port...
   [2/5] Waiting for connection to stabilize...
   [3/5] Clearing input/output buffers...
   [4/5] Sending GET_STATUS command...
   [5/5] Reading response...
   (No initial response. Retrying GET_STATUS query...)
   ❌ ERROR: No response to GET_STATUS query. Timed out.
      Ensure the Arduino has the correct sketch uploaded and is running.
   Closing serial port...
------------------------------------------------------------
🔍 Testing /dev/ttyACM0 (Nano 33 BLE)...
   [1/5] Opening serial port...
   [2/5] Waiting for connection to stabilize...
   [3/5] Clearing input/output buffers...
   [4/5] Sending GET_STATUS command...
   [5/5] Reading response...
   (No initial response. Retrying GET_STATUS query...)
   ❌ ERROR: Could not communicate with port: Write timeout
------------------------------------------------------------

============================================================
   VERIFICATION SUMMARY
============================================================
Total serial ports scanned: 2
Fully functional UWB nodes verified: 0
⚠️  Some nodes failed verification. Please review the errors above.
============================================================
this wont exit after a double exit ^C^C^C^C^CExce