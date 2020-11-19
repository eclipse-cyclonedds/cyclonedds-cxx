set(input ${INPUT})
set(output ${OUTPUT})

separate_arguments(INPUT)

# Create empty output file
file(WRITE ${output} "")
# Iterate through input files
foreach(bin IN LISTS INPUT)
  # Get short filename
  string(REGEX MATCH "([^/]+)$" filename ${bin})
  # Replace filename spaces & extension separator for C compatibility
  string(REGEX REPLACE "\\.| |-" "_" filename ${filename})
  # Read hex data from file
  file(READ ${bin} filedata HEX)
  # Convert hex data for C compatibility
  string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
  #replace windows line endings (CR+LF) with linux line endings (LF)
  string(REGEX REPLACE "0x0d,0x0a" "0x0a" filedata ${filedata})
  # Add terminating NULL to have the array be interchangeable with a string
  string(CONCAT filedata ${filedata} "0x0")
  # Append data to output file
  file(APPEND ${output} "const unsigned char ${filename}[] = {${filedata}};\nconst unsigned ${filename}_size = sizeof(${filename});\n")
endforeach()
