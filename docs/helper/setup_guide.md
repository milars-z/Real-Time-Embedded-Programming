# Raspberry Pi Setup and Maintenance Guide

## Step 1: OS Imaging
Tool: Raspberry Pi Imager
System: Raspberry Pi OS (64-bit)
Customization:
Set WiFi to match your PC.
Services: Enable SSH (Crucial for remote access).
Note: After flashing the OS onto the USB drive or SD card, it is normal for your PC to be unable to read the drive. This is due to the Linux file system format.

## Step 2: Initial Boot and Connection
Booting: Insert the drive into the Raspberry Pi and connect the power. The Pi will automatically connect to the configured WiFi (indicated by the green LED activity).
Verify Connection:
Open terminal/CMD on your PC and run: ping <hostname>.local
Remote Connection via SSH:
Connect using: ssh <username>@<hostname>.local or ssh <username>@<IP_ADDRESS>
When prompted, type yes to continue connecting.
Password Entry: Note that the password will not be visible as you type. Type it blindly and press Enter.
Development Essentials Installation:
Update the system: sudo apt update && sudo apt upgrade -y
Install build tools and debuggers:
sudo apt install build-essential cmake git gdb libgpiod-dev gpiod -y

## Step 3: VS Code Remote Development
Install the Remote - SSH extension in VS Code.
Click the Remote Window icon (bottom-left corner, looks like ><).
Select Connect to Host... -> Add New SSH Host...
Enter the same command used in the terminal: ssh <username>@<IP_ADDRESS>.
Follow the prompts to connect; the process is identical to the terminal connection.

## Step 4: Git Configuration
Identity Setup:
git config --global user.name "Your Name"
git config --global user.email "your_student_email@glasgow.ac.uk"
SSH Key Authentication (Passwordless Commits):
Generate Key: ssh-keygen -t ed25519 -C "your_github_email" (Press Enter for all prompts).
Retrieve Public Key: cat ~/.ssh/id_ed25519.pub
Copy the output (starting with ssh-ed25519) and add it to your GitHub account under Settings > SSH and GPG keys.
Verify Connection:
ssh -T git@github.com

## Step 5: Project Management
Clone Project:
Navigate to your project on GitHub, click Code > SSH, and copy the link.
In the Pi terminal, run: git clone <ssh_link>

## Daily Git Workflow:
Pull latest changes: git pull origin main
Commit changes:
git add .
git commit -m "feat: description of change"
git push origin <branch_name>
Branch Management:
Create and switch to a new branch: git checkout -b <branch_name>
Switch branches: git checkout <branch_name>
Check current branch: git branch

## Raspberry Pi Maintenance
### Power Button Logic (Raspberry Pi 5)
Single Click: Power On
Short Press (while running): Safe Shutdown
Long Press (5+ seconds): Forced Power Off (Use only if system is frozen)
### System Commands
Safe Shutdown: sudo shutdown now
System Reboot: sudo reboot
Network Management: sudo nmtui (Use this to backup/configure multiple WiFi networks to avoid losing access).
Temperature Monitor: vcgencmd measure_temp
Disk Usage: df -h






