/*
 adplug_adapter.js: Adapts AdPlug backend to generic WebAudio/ScriptProcessor player.
 
 version 1.0
 
 	Copyright (C) 2015 Juergen Wothke

 LICENSE
 
 This library is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or (at
 your option) any later version. This library is distributed in the hope
 that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
*/
AdPlugBackendAdapter = (function(){ var $this = function () { 
		$this.base.call(this, backend_AdPlug.Module, 2);	
	}; 
	// AdPlug's sample buffer contains 2-byte integer sample data (i.e. must be rescaled) 
	// of 2 interleaved channels
	extend(EmsHEAP16BackendAdapter, $this, {  
		getAudioBuffer: function() {
			var ptr=  this.Module.ccall('emu_get_audio_buffer', 'number');			
			// make it a this.Module.HEAP16 pointer
			return ptr >> 1;	// 2 x 16 bit samples			
		},
		getAudioBufferLength: function() {
			var len= this.Module.ccall('emu_get_audio_buffer_length', 'number') >>2;
			return len;
		},
		computeAudioSamples: function() {
			return this.Module.ccall('emu_compute_audio_samples', 'number');	
		},
		getMaxPlaybackPosition: function() { 
			return this.Module.ccall('emu_get_max_position', 'number');
		},
		getPlaybackPosition: function() {
			return this.Module.ccall('emu_get_current_position', 'number');
		},
		seekPlaybackPosition: function(pos) {
			this.Module.ccall('emu_seek_position', 'number', ['number'], [pos]);
		},

		getPathAndFilename: function(filename) {
			var sp = filename.split('/');
			var fn = sp[sp.length-1];
			var path= '/';	
			return [path, fn];
		},
		registerFileData: function(pathFilenameArray, data) {
			return this.registerEmscriptenFileData(pathFilenameArray, data);
		},
		loadMusicData: function(sampleRate, path, filename, data, options) {
			var ret = this.Module.ccall('emu_init', 'number', ['number', 'string', 'string'], 
														[sampleRate, path, filename]);

			if (ret == 0) {			
				var inputSampleRate = sampleRate;
				this.resetSampleRate(sampleRate, inputSampleRate); 
			}
			return ret;			
		},
		evalTrackOptions: function(options) {
			if (typeof options.timeout != 'undefined') {
				ScriptNodePlayer.getInstance().setPlaybackTimeout(options.timeout*1000);
			}
			var track= options.track;
			var ret= this.Module.ccall('emu_set_subsong', 'number', ['number'], [track]);
			return ret;
		},				
		teardown: function() {
			this.Module.ccall('emu_teardown', 'number');	// just in case
		},
		getSongInfoMeta: function() {
			return {title: String,
					author: String, 
					desc: String, 
					player: String, 
					speed: Number, 
					tracks: Number
					};
		},
		updateSongInfo: function(filename, result) {
			// get song infos
			var numAttr= 6;
			var ret = this.Module.ccall('emu_get_track_info', 'number');

			var array = this.Module.HEAP32.subarray(ret>>2, (ret>>2)+numAttr);
			result.title= this.Module.Pointer_stringify(array[0]);
			if (!result.title.length) result.title= filename;		
			result.author= this.Module.Pointer_stringify(array[1]);		
			result.desc= this.Module.Pointer_stringify(array[2]);
			result.player= this.Module.Pointer_stringify(array[3]);
			var s= parseInt(this.Module.Pointer_stringify(array[4]))
			result.speed= s;
			var t= parseInt(this.Module.Pointer_stringify(array[5]))
			result.tracks= t;
		}
	});	return $this; })();
	