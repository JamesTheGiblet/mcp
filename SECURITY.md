# Security Setup Guide

## 🔒 **IMPORTANT: Protecting Your Credentials**

This project contains sensitive information that should NOT be shared publicly on GitHub:

- WiFi passwords
- API keys
- Server IP addresses

## 🛡️ **Security Setup Steps**

### 1. **Copy Template to Create Your Config**

```bash
cd esp32_examples/bots/wheelie_bot/src/
cp config_template.h config.h
```

### 2. **Edit Your Config File**

Open `config.h` and replace these placeholder values with your actual credentials:

```cpp
// Replace with your actual WiFi credentials
const char* WIFI_SSID = "YOUR_WIFI_SSID_HERE";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD_HERE";

// Replace with your server IP
const char* MCP_SERVER_IP_FALLBACK = "192.168.1.100";

// Replace with secure keys
const char* MCP_API_KEY = "CHANGE_THIS_API_KEY_FOR_SECURITY";
const char* OTA_PASSWORD = "CHANGE_THIS_OTA_PASSWORD_FOR_SECURITY";
```

### 3. **Verify Security**

Your `config.h` file is automatically protected by `.gitignore` and will NOT be uploaded to GitHub.

## ✅ **What Gets Committed to GitHub**

- ✅ `config_template.h` - Safe template with placeholders
- ✅ All source code and documentation
- ✅ Project structure and build files

## ❌ **What Stays Private**

- ❌ `config.h` - Your actual credentials (protected by .gitignore)
- ❌ Any files containing real passwords or API keys

## 🔍 **Security Verification**

Before committing to GitHub, always check:

```bash
git status
```

Make sure `config.h` does NOT appear in the list of files to be committed.

If `config.h` appears, it means the .gitignore isn't working properly.

## 🚨 **If You Accidentally Commit Credentials**

If you accidentally commit real credentials:

1. **Change all passwords immediately**
2. **Generate new API keys**
3. **Remove the sensitive commit from Git history**
4. **Force push the cleaned history**

## 🎯 **Current Security Status**

✅ **SECURE**: Your current `config.h` is protected by `.gitignore`  
✅ **SECURE**: Template file created for public sharing  
✅ **SECURE**: Sensitive credential patterns excluded from Git  

Your project is now ready for GitHub while keeping your credentials private!

---
**Remember**: Never share your actual WiFi passwords or API keys in public repositories!
