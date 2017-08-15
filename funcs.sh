function clonepull {
		[[ -n "$dest" ]] || return 1
		if [[ -d "$local" ]]; then
        adjremote=1
    else
				[[ -n "$remote" ]] || return 2
        # echo source not found, using remote source $remote
    fi

		if [[ -d "$dest" ]]; then
        [[ -n "$nocheck" ]] && return
        cd $dest
        git pull local master
        git pull origin  master
        cd ..
    else
				if [[ -n "$adjremote" ]]; then
						git clone --recursive $local $dest
				else
						git clone --recursive $remote $dest
				fi
        if [[ -n "$adjremote" ]]; then
            cd $dest
            git remote set-url origin $remote
            git remote add local $source
            #git pull
            cd ..
        fi
    fi
}

function uplink {
		# this is better than thrice-cloned submodules
		# uplink dir name => name links to dir/name
    source=$1/$2
    [[ -L $2 ]] && return;
    ln -rs $source $2
}
