#write pyhon code that produces a file called allTheCode.txt by going in the current  directory and for each .cpp and .h file puts the file name in # the allTheCode.txt and then on the next lines the content of the file
import os
import datetime

def combine_code_files(output_filename="allTheCode.txt"):
    """
    Scans the current directory for .cpp and .h files, combines their 
    filenames and content into a single output file, overwriting if 
    the file exists, and adds a header with current time and date.
    """
    
    extensions = ('.cpp', '.h')
    
    # Open the output file in 'w' (write) mode, which overwrites existing files
    try:
        with open(output_filename, 'w', encoding='utf-8') as outfile:
            
            # --- Add the required header ---
            current_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            outfile.write("Here is all the current code:\n")
            outfile.write(f"at: {current_time}\n")
            outfile.write("="*40 + "\n\n")

            # Iterate through all files in the current directory
            for filename in os.listdir('.'):
                if filename.lower().endswith(extensions):
                    print(f"Processing file: {filename}")
                    
                    try:
                        with open(filename, 'r', encoding='utf-8') as infile:
                            content = infile.read()
                            
                            # Write the filename header
                            outfile.write(f"--- FILE: {filename} ---\n\n")
                            # Write the file content
                            outfile.write(content)
                            # Add a couple of newlines for separation before the next file
                            outfile.write("\n\n") 
                            
                    except IOError as e:
                        print(f"Error reading file {filename}: {e}")
                    except UnicodeDecodeError as e:
                        print(f"Error decoding file {filename} (non-UTF-8 characters?): {e}")

        print(f"\nSuccessfully created '{output_filename}' with combined code.")

    except IOError as e:
        print(f"Error writing to output file {output_filename}: {e}")

if __name__ == "__main__":
    combine_code_files()
