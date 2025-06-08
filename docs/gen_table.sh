# chmod +x generate_github_release_table.py
# export GITHUB_TOKEN="ghp_xxxxxxxx..."

python generate_github_release_table.py postechsv/maude-se source/wheels.md 1 .whl,.tar.gz "Wheels"
python generate_github_release_table.py postechsv/maude-se source/native.md 1 .zip "Executables"