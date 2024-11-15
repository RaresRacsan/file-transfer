# Security Policy

## Supported Versions

The following versions of the *File Transfer Application in C (TCP)* project are currently supported with security updates:

| Version | Supported          |
|---------|--------------------|
| v1.0    | :white_check_mark: |

If you are using an unsupported version, we recommend upgrading to the latest version to ensure you receive important updates.

---

## Reporting a Vulnerability

If you discover a security vulnerability in this project, please report it responsibly by following these steps:

1. **Contact Us**: Send an email to [your_email@example.com] with the subject line: `Security Issue: [Brief Description]`.
2. **Provide Details**: Include as much detail as possible to help us reproduce and understand the issue. 
   - Steps to reproduce the vulnerability.
   - Potential impact of the issue.
   - Suggestions for mitigating the problem (if any).
3. **Avoid Public Disclosure**: Please refrain from publicly disclosing the vulnerability until we have confirmed and addressed it.

We take all reports seriously and will work to resolve any security issues promptly.

---

## Security Patch Process

1. Upon receiving a report, we will:
   - Acknowledge the report within 48 hours.
   - Investigate and validate the vulnerability.
   - Assign a severity rating (Low, Medium, High, Critical).
2. If the issue is valid:
   - Develop and test a fix.
   - Publish a patch in a new release.
   - Credit the reporter (if they agree) in the release notes.
3. Notify affected users and update the repository.

---

## Security Best Practices for Contributors

If you're contributing to this project, please follow these guidelines to maintain security:
1. **Avoid Hardcoding Sensitive Data**: Do not include hardcoded credentials, keys, or other sensitive data in the codebase.
2. **Sanitize User Input**: Ensure all input is validated and sanitized to prevent vulnerabilities like buffer overflows.
3. **Follow Secure Coding Practices**: Adhere to industry best practices for secure C programming, including:
   - Using safe functions like `fgets` instead of `gets`.
   - Preventing memory leaks and improper memory access.
4. **Review Changes Carefully**: Thoroughly test and review your changes before submitting pull requests.

---

## Acknowledgments

We greatly appreciate responsible disclosures from the security community. Thank you for helping us maintain a secure and reliable application.

---

**Last Updated:** [15.11.2024]
