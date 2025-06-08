#!/usr/bin/env python3
import os
import sys
import requests
from datetime import datetime

def generate_release_table(repo, output_file, artifact_whitelist=None, num_latest=1, artifact_column_name="Artifacts"):
    output_dir = os.path.dirname(output_file)
    os.makedirs(output_dir, exist_ok=True)

    url = f"https://api.github.com/repos/{repo}/releases"
    headers = {}
    github_token = os.environ.get("GITHUB_TOKEN")
    if github_token:
        headers["Authorization"] = f"token {github_token}"

    response = requests.get(url, headers=headers)
    print("GitHub API status:", response.status_code)

    if response.status_code != 200:
        print("Failed to fetch release info.")
        sys.exit(1)

    releases = response.json()

    if not releases:
        print("No releases found.")
        sys.exit(1)

    # exclude pre-release
    stable_releases = [r for r in releases if not r.get("prerelease", False)]
    if not stable_releases:
        print("No stable releases found.")
        sys.exit(1)

    # extract latest N
    latest_releases = stable_releases[:num_latest]

    with open(output_file, "w") as f:
        f.write(f"Release | Published At | {artifact_column_name}\n")
        f.write("---|---|:---:\n")

        for release in latest_releases:
            name = release.get("name") or release.get("tag_name", "")

            published_iso = release.get("published_at", "")
            if published_iso:
                dt = datetime.fromisoformat(published_iso.replace("Z", "+00:00"))
                published = dt.strftime("%Y-%m-%d")
            else:
                published = "—"

            assets = release.get("assets", [])
            if assets:
                filtered_assets = []
                for asset in assets:
                    asset_name = asset['name']
                    if artifact_whitelist is None or any(asset_name.endswith(ext) for ext in artifact_whitelist):
                        asset_link = f"[{asset_name}]({asset['browser_download_url']})"
                        filtered_assets.append(asset_link)
                asset_list = "<br>".join(filtered_assets) if filtered_assets else "—"
            else:
                asset_list = "—"

            f.write(f"{name} | {published} | {asset_list}\n")

    print(f"Release table generated at {output_file}!")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python generate_github_release_table.py <repo> <output_file> [<num_latest>] [<artifact_ext1>,<artifact_ext2>,...] [<artifact_column_name>]")
        sys.exit(1)

    repo = sys.argv[1]
    output_file = sys.argv[2]
    num_latest = int(sys.argv[3]) if len(sys.argv) >= 4 else 1
    artifact_whitelist = sys.argv[4].split(",") if len(sys.argv) >= 5 and sys.argv[4] else None
    artifact_column_name = sys.argv[5] if len(sys.argv) >= 6 else "Artifacts"

    generate_release_table(
        repo,
        output_file,
        artifact_whitelist=artifact_whitelist,
        num_latest=num_latest,
        artifact_column_name=artifact_column_name
    )
