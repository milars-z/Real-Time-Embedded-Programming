# Raspberry Pi Setup and Maintenance Guide

## Step 1: OS Imaging
Tool: Raspberry Pi Imager  
System: Raspberry Pi OS (64-bit)  
Customization: Set WiFi to match your PC.  
Services: Enable SSH (Crucial for remote access).  
Note: After flashing the OS onto the USB drive or SD card, it is normal for your PC to be unable to read the drive. This is due to the Linux file system format.

## Step 2: Initial Boot and Connection
Booting: Insert the drive into the Raspberry Pi and connect the power. The Pi will automatically connect to the configured WiFi (indicated by the green LED activity).  
Verify Connection:  
Open terminal/CMD on your PC and run: 
```bash
    ping <hostname>.local  
```
Remote Connection via SSH:  
Connect using: 
```bash
    ssh <username>@<hostname>.local 
    ssh <username>@<IP_ADDRESS>  
```
When prompted, type yes to continue connecting.  
Password Entry: Note that the password will not be visible as you type. Type it blindly and press Enter.  
Development Essentials Installation:  
Update the system: 
```bash
    sudo apt update && sudo apt upgrade -y  
```
Install build tools and debuggers:  
```bash
    sudo apt install build-essential cmake git gdb libgpiod-dev gpiod -y  
```

## Step 3: VS Code Remote Development
Install the Remote - SSH extension in VS Code.  
Click the Remote Window icon (bottom-left corner, looks like ><).  
Select Connect to Host... -> Add New SSH Host...  
Enter the same command used in the terminal: 
```bash
    ssh <username>@<IP_ADDRESS>.  
```
Follow the prompts to connect; the process is identical to the terminal connection.  

## Step 4: Git Configuration
Identity Setup:  
```bash
    git config --global user.name "Your Name"  
    git config --global user.email "your_student_email@glasgow.ac.uk"  
```
SSH Key Authentication (Passwordless Commits):  
Generate Key: 
```bash
    ssh-keygen -t ed25519 -C "your_github_email" (Press Enter for all prompts).  
```
Retrieve Public Key:
```bash 
    cat ~/.ssh/id_ed25519.pub  
```
Copy the output (starting with ssh-ed25519) and add it to your GitHub account under Settings > SSH and GPG keys.  
Verify Connection:  
```bash
    ssh -T git@github.com  
```

## Step 5: Project Management
Clone Project:  
Navigate to your project on GitHub, click Code > SSH, and copy the link.  
In the Pi terminal, run: 
```bash
    git clone <ssh_link>
```  

## Daily Git Workflow:
Pull latest changes: 
```bash
    git pull origin main  
```
Commit changes:  
```bash
    git add .  
    git commit -m "feat: description of change"  
    git push origin <branch_name>  
```
Branch Management:  
  Create and switch to a new branch: 
```bash
    git checkout -b <branch_name>  
```
  Switch branches: 
```bash
    git checkout <branch_name> 
``` 
  Check current branch: 
```bash
    git branch  
```

## Raspberry Pi Maintenance
### Power Button Logic (Raspberry Pi 5)
Single Click: Power On  
Short Press (while running): Safe Shutdown  
Long Press (5+ seconds): Forced Power Off (Use only if system is frozen)  
### System Commands
Safe Shutdown: 
```bash
    sudo shutdown now
```  
System Reboot:
```bash
    sudo reboot
```  
Network Management:  
(Use this to backup/configure multiple WiFi networks to avoid losing access).  
```bash
    sudo nmtui   
```
Temperature Monitor:
```bash
    vcgencmd measure_temp  
```
Disk Usage:
```bash
    df -h
```  






