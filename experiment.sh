for file in "test_cases"/*; do
    if [ -f "$file" ]; then
        filename=$(basename -- "$file")
        echo $filename
        
        ./a.out "test_cases/$filename" > "results/$filename"
        
        echo ""
    fi
done
