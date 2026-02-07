#!/bin/bash
set -e

# This script creates GitHub issues for open CodeQL security alerts
# It queries the GitHub API for code scanning alerts and creates an issue for each one
# that doesn't already have an associated issue

REPO="${GITHUB_REPOSITORY}"
ALERT_STATE="open"

echo "Fetching open CodeQL alerts for ${REPO}..."

# Fetch all open code scanning alerts
ALERTS=$(gh api \
  -H "Accept: application/vnd.github+json" \
  -H "X-GitHub-Api-Version: 2022-11-28" \
  "/repos/${REPO}/code-scanning/alerts?state=${ALERT_STATE}" \
  --jq '.[] | {number: .number, rule_id: .rule.id, rule_name: .rule.name, rule_description: .rule.description, severity: .rule.severity, html_url: .html_url, created_at: .created_at, location: .most_recent_instance.location}')

if [ -z "$ALERTS" ]; then
  echo "No open alerts found."
  exit 0
fi

# Process each alert
echo "$ALERTS" | jq -c '.' | while IFS= read -r alert; do
  ALERT_NUMBER=$(echo "$alert" | jq -r '.number')
  RULE_ID=$(echo "$alert" | jq -r '.rule_id')
  RULE_NAME=$(echo "$alert" | jq -r '.rule_name')
  RULE_DESC=$(echo "$alert" | jq -r '.rule_description')
  SEVERITY=$(echo "$alert" | jq -r '.severity')
  ALERT_URL=$(echo "$alert" | jq -r '.html_url')
  CREATED_AT=$(echo "$alert" | jq -r '.created_at')
  LOCATION_PATH=$(echo "$alert" | jq -r '.location.path // "N/A"')
  LOCATION_LINE=$(echo "$alert" | jq -r '.location.start_line // "N/A"')
  
  echo ""
  echo "Processing Alert #${ALERT_NUMBER}: ${RULE_NAME} (${SEVERITY})"
  
  # Check if an issue already exists for this alert
  # Search for issues with the alert number in the title
  EXISTING_ISSUE=$(gh issue list \
    --label "codeql" \
    --state all \
    --search "CodeQL Alert #${ALERT_NUMBER}" \
    --json number \
    --jq '.[0].number // empty')
  
  if [ -n "$EXISTING_ISSUE" ]; then
    echo "  Issue already exists: #${EXISTING_ISSUE}"
    continue
  fi
  
  # Create the issue title
  ISSUE_TITLE="[Security] CodeQL Alert #${ALERT_NUMBER}: ${RULE_DESC}"
  
  # Create the issue body
  ISSUE_BODY=$(cat <<EOF
## Security Alert from CodeQL

**Alert Number:** #${ALERT_NUMBER}
**Rule ID:** \`${RULE_ID}\`
**Rule Name:** \`${RULE_NAME}\`
**Severity:** **${SEVERITY}**
**Created:** ${CREATED_AT}

### Description
${RULE_DESC}

### Location
- **File:** \`${LOCATION_PATH}\`
- **Line:** ${LOCATION_LINE}

### View Alert
🔗 [View full alert details on GitHub](${ALERT_URL})

---
*This issue was automatically created by the CodeQL workflow.*
EOF
)
  
  # Determine labels based on severity
  LABELS="security,codeql,severity:${SEVERITY}"
  
  # Create the issue
  echo "  Creating issue..."
  ISSUE_NUMBER=$(gh issue create \
    --title "$ISSUE_TITLE" \
    --body "$ISSUE_BODY" \
    --label "$LABELS" \
    --json number \
    --jq '.number')
  
  echo "  ✅ Created issue #${ISSUE_NUMBER}"
done

echo ""
echo "Done processing alerts."
