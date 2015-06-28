#!/bin/bash
# retag.sh -- Tag the adaptit git repository for adaptit version provided as parameter
#             if the version number already exists it retags repository to the same version.
#          -- Assumes the current directory is the local active adaptit git repository.
#
# Author: Bill Martin <bill_martin@sil.org>
#
# Date: 2015-05-27

TAGRETAGSCRIPT="tagretag.sh"
WAIT=60

# A bash function to verify a git tag
verify_git_tag()
{
    echo -e "\nVerifying git tag: \"$1\" ...."
    if git branch -r --contains $(git rev-parse $1) | grep origin>/dev/null 2>&1
    then
        return 0
    else
        return 1
    fi
}

# A bash function to verify that current dir is a git repository
is_git_repo()
{
    if git status | grep "fatal: Not a git repository">/dev/null 2>&1
    then
        return 1
    else
        return 0
    fi
}

# ------------------------------------------------------------------------------
# Main program starts here
# ------------------------------------------------------------------------------
echo -e "\nNumber of parameters: $#"
echo -e "\n"
case $# in
    0) 
      echo "The $TAGRETAGSCRIPT script was invoked without any parameters:"
      echo "Usage: Call with one parameter representing the git tag, i.e.,"
      echo "  ./tagretag.sh adaptit-6.5.8"
      exit 1
        ;;
    1) 
      COPYTODIR="$1"
      echo "The $TAGRETAGSCRIPT script was invoked with 1 parameter: $1"
        ;;
    *)
      echo "Unrecognized parameters used with script."
      echo "Usage: Call with one parameter representing the git tag, i.e.,"
      echo "  ./tagretag.sh adaptit-6.5.8"
      exit 1
        ;;
esac

#echo "Debug Breakpoint - next: verify git repository"
#exit 0

# First check that the current dir is a git repository
    echo -e "\nVerifying git repository...."
if is_git_repo ; then
    echo "`pwd` is a git repository."
else
    echo "`pwd` is NOT a git repository."
    echo "Please cd to a valid git repository. Aborting..."
    exit 1
fi

#echo "Debug Breakpoint - next: call git pull"
#exit 0

# Next execute a git pull to be sure our git repo is up-to-date
echo "Executing git pull..."
git pull

#echo "Debug Breakpoint - next: call verify_git_tag()"
#exit 0

if verify_git_tag "$1" ; then
    # The input tag name-version was found
    echo "Git tag \"$1\" already exists..."

#echo "Debug Breakpoint - next: delete existing tag $1"
#exit 0

    # Delete the existing tag
    echo -e "\nDelete the existing tag: \"$1\"? [y/n]"
    for (( i=$WAIT; i>0; i--)); do
        printf "\rPlease press the y or n key, or hit any key to abort - countdown $i "
        read -s -n 1 -t 1 response1
        if [ $? -eq 0 ]
        then
            break
        fi
    done
    if [ ! $response1 ]; then
      echo -e "\nNo selection made, or no reponse within $WAIT seconds. Assuming response of n"
      response1="n"
    fi
    echo -e "\nYour choice was $response1"
    case $response1 in
      [yY][eE][sS]|[yY]) 
          echo -e "\nDeleting the git repository tag: $1 ..."
          git tag -d "$1" 
          git push origin :refs/tags/"$1"

#echo "Debug Breakpoint - next: retag the repo with the tag $1"
#exit 0
          echo -e "\nTag the git repository with the new tag: \"$1\"? [y/n]"
          for (( i=$WAIT; i>0; i--)); do
              printf "\rPlease press the y or n key, or hit any key to abort - countdown $i "
              read -s -n 1 -t 1 response2
              if [ $? -eq 0 ]
              then
                  break
              fi
          done
          if [ ! $response2 ]; then
            echo -e "\nNo selection made, or no reponse within $WAIT seconds. Assuming response of n"
            response2="n"
          fi
          echo -e "\nYour choice was $response2"
          case $response2 in
            [yY][eE][sS]|[yY]) 
                echo -e "\nTagging the git repository with the tag: $1 ..."
                git tag -a "$1" -m "Tagging $1"
                git push origin "$1"
                ;;
             *)
                echo -e "\nNo action taken! Aborting..."
                exit 0
                ;;
          esac
          ;;
       *)
          echo -e "\nNo action taken! Aborting..."
          exit 0
          ;;
    esac

else
    # The input tag name-version was not found
    echo "The \"$1\" tag is not in the repository."

#echo "Debug Breakpoint - next: tag the repo with the new tag $1"
#exit 0

    echo -e "\nTag the git repository with the new tag: \"$1\"? [y/n]"
    for (( i=$WAIT; i>0; i--)); do
        printf "\rPlease press the y or n key, or hit any key to abort - countdown $i "
        read -s -n 1 -t 1 response2
        if [ $? -eq 0 ]
        then
            break
        fi
    done
    if [ ! $response2 ]; then
      echo -e "\nNo selection made, or no reponse within $WAIT seconds. Assuming response of n"
      response2="n"
    fi
    echo -e "\nYour choice was $response2"
    case $response2 in
      [yY][eE][sS]|[yY]) 
          echo -e "\nTagging the git repository with the tag: $1 ..."
          git tag -a "$1" -m "Tagging $1"
          git push origin "$1"
          ;;
       *)
          echo -e "\nNo action taken! Aborting..."
          exit 0
          ;;
    esac
fi
echo "The $TAGRETAGSCRIPT script has finished!"

 
