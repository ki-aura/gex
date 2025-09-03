
for file in *.h; do


sed -i ''  's/goto_byte/v_goto_byte/g' "$file"
sed -i '' 's/handle_view_keys/v_handle_keys/g' "$file"
sed -i '' 's/populate_grids/v_populate_grids/g' "$file"
sed -i '' 's/refresh_ascii/v_refresh_ascii/g' "$file"
sed -i '' 's/refresh_hex/v_refresh_hex/g' "$file"

done


