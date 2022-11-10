#!/bin/bash
        #convert $i -resize 50% -quality 25% converted/$(basename $i .$image_type).jpg
        #convert $i -resize 50% -quality 25% $(basename $i .$image_type).jpg
        #100x75 randori preview
        #1024x760 randori max allowed resolution
#mkdir converted 
image_type="jpg"
count=0
echo "c[options [in order] ]"
echo "f - set folder"
echo "s - set preferences"
echo "r - rename"
echo "empty - default"
default_size=66
default_quality=60

read command

if [ "$command" == "cf" -o "$command" == "cfr" ]; then
    echo Enter folder name
    read dirname
    mkdir $dirname
    
elif [ "$command" == "cs" -o "$command" == "cfs" -o "$command" == "cfsr" -o "$command" == "csr" ]; then
    echo "dimension in [p]ercent or pi[x]els?"
    read dimension
    if [ "$dimension" == "x" ]; then
        echo "dimensions in pixels(XxY, ex.: 300x400)"
        read size
        
    elif [ "$dimension" == "p" ]; then
        echo "dimension in percent(0-100%)"
        read size
    fi  
    
    echo "quality in percent(0-100%)"
    read quality
    
    if [ "$command" == "cfs" ]; then
        echo Enter folder name
        read dirname
        mkdir $dirname
    fi
    
fi

for i in `ls | grep -i $image_type`; 
    do 
        if [ "$command" == "cr" -o "$command" == "csr" -o "$command" == "cfr" -o "$command" == "cfsr" -o "$command" == "r" ]; then
            img_name=`echo $i | sed 's/.jpg//'`
            img_name=`echo $i | sed 's/.JPG//'`
            img_name=`echo $img_name\_$count$RANDOM.$image_type`
        else img_name=$i
        fi
        
        if [ "$command" == "c" -o "$command" == "cr" ]; then    
            convert $i -resize $default_size% -quality $default_quality% $img_name
            
        elif [ "$command" == "cf" -o "$command" == "cfr" ]; then            
            convert $i -resize $default_size% -quality $default_quality% $dirname/$img_name
        
        elif [ "$command" == "cs" -o "$command" == "csr" ]; then            
            if [ "$dimension" == "x" ]; then
                convert $i -resize $size -quality $quality% $img_name
                
            elif [ "$dimension" == "p" ]; then
                convert $i -resize $size% -quality $quality% $img_name
            fi
            
        elif [ "$command" == "cfs" -o "$command" == "cfsr" ]; then  
            if [ "$dimension" == "x" ]; then
                convert $i -resize $size -quality $quality% $dirname/$img_name
                
            elif [ "$dimension" == "p" ]; then
                convert $i -resize $size% -quality $quality% $dirname/$img_name
            fi
                
        elif [ "$command" == "r" ]; then
            mv $i $img_name         
        fi
        let count+=1
done
