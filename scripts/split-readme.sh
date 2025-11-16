#!/usr/bin/env bash

# SPDX-License-Identifier: Apache-2.0
# Copyright Contributors to the OpenQMC Project.

set -e

# Split README.md and CONTRIBUTING.md into multiple markdown files for MkDocs
# based on markers and fetch GitHub releases Usage: ./scripts/split-readme.sh
#
# This script extracts sections from README.md and CONTRIBUTING.md marked
# with HTML comment markers and generates separate markdown files in the
# docs/ directory for MkDocs. It also fetches GitHub releases and generates a
# releases.md page. Generated files are gitignored and created fresh on each
# CI build. Image paths are rewritten from ./images/ to ../images/ during
# extraction.

readonly README_FILE="README.md"
readonly CONTRIBUTING_FILE="CONTRIBUTING.md"
readonly DOCS_DIR="docs"
readonly RELEASES_LIMIT=20

# Declare anchor map at top level for compatibility
declare -A ANCHOR_MAP

# Fetch GitHub releases and generate a releases.md page
fetch_releases() {
    local output_file="$DOCS_DIR/releases.md"

    echo "Fetching GitHub releases..."

    # Check if gh CLI is available
    if ! command -v gh &> /dev/null; then
        echo "Warning: gh CLI not found, skipping releases page generation"
        return 0
    fi

    # Start the releases page
    cat > "$output_file" << 'EOF'
# Releases

All releases of the OpenQMC project. For the GitHub release page, see [here](https://github.com/AcademySoftwareFoundation/openqmc/releases).

EOF

    # Get repository URL
    local repo_url
    repo_url=$(gh repo view --json url -q '.url')

    # Fetch releases and format them with their notes
    gh release list --limit "$RELEASES_LIMIT" --json tagName,name,publishedAt,isLatest | \
    jq -r '.[] | [.tagName, .name, .publishedAt, .isLatest] | @tsv' | \
    while IFS=$'\t' read -r tag name published isLatest; do
        echo "  Fetching $tag..."

        # Construct release URL
        local release_url="${repo_url}/releases/tag/${tag}"

        # Format the heading
        local display_name="${name:-$tag}"
        echo "## [$display_name]($release_url)" >> "$output_file"
        echo "" >> "$output_file"

        # Add latest badge if applicable
        if [ "$isLatest" = "true" ]; then
            echo "> **Latest Release**" >> "$output_file"
            echo "" >> "$output_file"
        fi

        # Format the date (extract just the date part from ISO format)
        local date_formatted
        date_formatted=$(echo "$published" | cut -d'T' -f1)
        echo "**Released:** $date_formatted" >> "$output_file"
        echo "" >> "$output_file"

        # Get release body and convert H3 headings to bold text to keep them out of TOC
        local body
        body=$(gh release view "$tag" --json body -q '.body')

        if [ -n "$body" ]; then
            # Strip carriage returns, convert ### Heading to **Heading**, and trim trailing spaces
            echo "$body" | tr -d '\r' | sed -E 's/^### (.+)$/\n**\1**\n/' | sed -E 's/ +\*\*/**/g' >> "$output_file"
        else
            echo "*No release notes available.*" >> "$output_file"
        fi

        echo "" >> "$output_file"
        echo "---" >> "$output_file"
        echo "" >> "$output_file"
    done

    local count
    count=$(gh release list --limit "$RELEASES_LIMIT" | wc -l | tr -d ' ')
    echo "Generated releases page with $count releases"
}

# Build a map of anchor IDs to their containing files
# This allows us to fix cross-page anchor links
build_anchor_map() {
    for file in "$DOCS_DIR"/*.md; do
        local basename;
        basename=$(basename "$file")

        # Extract headings and convert to anchor IDs (lowercase, spaces->hyphens, remove special chars)
        while IFS= read -r line; do
            if [[ $line =~ ^#+[[:space:]]+(.*) ]]; then
                local heading="${BASH_REMATCH[1]}"
                # Convert heading to anchor ID (MkDocs/markdown style)
                local anchor;
                anchor=$(echo "$heading" | tr '[:upper:]' '[:lower:]' | sed 's/[^a-z0-9 -]//g' | tr ' ' '-')
                ANCHOR_MAP["$anchor"]="$basename"
            fi
        done < "$file"
    done
}

# Convert relative source code links to full GitHub URLs
fix_source_links() {
    local github_base="https://github.com/AcademySoftwareFoundation/openqmc/blob/main"

    for file in "$DOCS_DIR"/*.md; do
        # Create temp file for modifications
        local temp_file="${file}.tmp"

        while IFS= read -r line; do
            # Replace relative paths with full GitHub URLs for source code links
            line=$(echo "$line" | sed -E "s|\]\((src/[^)]+)\)|](${github_base}/\1)|g")
            line=$(echo "$line" | sed -E "s|\]\((include/[^)]+)\)|](${github_base}/\1)|g")
            line=$(echo "$line" | sed -E "s|\]\((python/[^)]+)\)|](${github_base}/\1)|g")
            line=$(echo "$line" | sed -E "s|\]\((cmake/[^)]+)\)|](${github_base}/\1)|g")
            line=$(echo "$line" | sed -E "s|\]\((doxygen/[^)]+)\)|](${github_base}/\1)|g")
            # Handle CHANGELOG.md and other markdown files at root
            line=$(echo "$line" | sed -E "s|\]\(CHANGELOG\.md\)|](${github_base}/CHANGELOG.md)|g")
            line=$(echo "$line" | sed -E "s|\]\(SUPPORT\.md\)|](${github_base}/SUPPORT.md)|g")
            line=$(echo "$line" | sed -E "s|\]\(GOVERNANCE\.md\)|](${github_base}/GOVERNANCE.md)|g")
            echo "$line"
        done < "$file" > "$temp_file"

        mv "$temp_file" "$file"
    done
}

# Fix cross-page anchor references in all generated files
fix_cross_references() {
    for file in "$DOCS_DIR"/*.md; do
        local basename;
        basename=$(basename "$file")

        # Create temp file for modifications
        local temp_file="${file}.tmp"

        while IFS= read -r line; do
            # Find markdown links with anchor-only references: [text](#anchor)
            if [[ $line =~ \]\(#([a-z0-9-]+)\) ]]; then
                local anchor="${BASH_REMATCH[1]}"

                # Check if anchor exists in map and is in a different file
                if [[ -n "${ANCHOR_MAP[$anchor]}" && "${ANCHOR_MAP[$anchor]}" != "$basename" ]]; then
                    local target_file="${ANCHOR_MAP[$anchor]}"

                    line=$(echo "$line" | sed "s|](#${anchor})|](${target_file}#${anchor})|g")
                fi
            fi
            echo "$line"
        done < "$file" > "$temp_file"

        mv "$temp_file" "$file"
    done
}

# Add front matter to specific files
add_front_matter() {
    local files_to_modify=("releases.md")

    for filename in "${files_to_modify[@]}"; do
        local file="$DOCS_DIR/$filename"

        if [ -f "$file" ]; then
            local temp_file="${file}.tmp"

            # Add front matter at the beginning
            cat > "$temp_file" << 'EOF'
---
hide:
  - navigation
---

EOF
            # Append original content
            cat "$file" >> "$temp_file"
            mv "$temp_file" "$file"

            echo "Added front matter to $filename"
        fi
    done
}

# Remove any existing generated markdown files to prevent duplication
rm -rf "$DOCS_DIR"

# Ensure docs directory exists
mkdir -p "$DOCS_DIR"

# Process a markdown file with MKDOCS_SPLIT markers
process_markdown_file() {
    local source_file="$1"
    local file_display_name="$2"

    if [ ! -f "$source_file" ]; then
        echo "Warning: $source_file not found, skipping"
        return 0
    fi

    echo "Processing $file_display_name..."

    # Use awk to parse markers and split content
    awk -v docs_dir="$DOCS_DIR" '
    BEGIN {
        current_file = ""
    }

    # Handle regular split markers
    /<!-- MKDOCS_SPLIT: / {
        # Close previous file if open
        if (current_file) close(current_file)

        # Extract filename from marker
        match($0, /<!-- MKDOCS_SPLIT: ([^ ]+) -->/, arr)
        current_file = docs_dir "/" arr[1]
        next
    }

    # Handle end marker
    /<!-- MKDOCS_SPLIT_END -->/ {
        if (current_file) close(current_file)
        current_file = ""
        next
    }

    # Write content to current file, rewriting image paths for MkDocs
    current_file {
        line = $0
        gsub(/\(\.\/images\//, "(../images/", line)
        gsub(/srcset="\.\/images\//, "srcset=\"../images/", line)
        gsub(/src="\.\/images\//, "src=\"../images/", line)
        print line >> current_file
    }
    ' "$source_file"
}

echo "Splitting README.md and CONTRIBUTING.md into MkDocs files..."

# Create index.md with front matter for custom homepage
cat > "$DOCS_DIR/index.md" << 'EOF'
---
template: home.html
---
EOF

# Create symlink for images directory so MkDocs can find and copy them
if [ ! -e "$DOCS_DIR/images" ]; then
    ln -s ../images "$DOCS_DIR/images"
    echo "Created symlink: docs/images -> ../images"
fi

# Create symlink to mkdocs directory so extra_javascript paths work
if [ ! -e "$DOCS_DIR/javascripts" ]; then
    ln -s ../mkdocs/javascripts "$DOCS_DIR/javascripts"
    echo "Created symlink: docs/javascripts -> ../mkdocs/javascripts"
fi

# Process both README.md and CONTRIBUTING.md
process_markdown_file "$README_FILE" "README.md"
process_markdown_file "$CONTRIBUTING_FILE" "CONTRIBUTING.md"

echo "Successfully processed markdown files into docs/"

# Fetch GitHub releases
fetch_releases

# Fix source code links to point to GitHub
echo "Converting source code links to GitHub URLs..."
fix_source_links
echo "Converted source code links"

# Build anchor map and fix cross-page references
echo "Fixing cross-page anchor links..."
build_anchor_map
fix_cross_references
echo "Fixed cross-page anchor links"

# Add front matter to hide navigation on specific pages
echo "Adding front matter to specific pages..."
add_front_matter
echo "Added front matter"

ls -1 "$DOCS_DIR"/*.md 2>/dev/null || echo "No markdown files generated"
