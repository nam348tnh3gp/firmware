<script>
	import { onMount } from 'svelte';
	
	let showBanner = $state(true);
	let autoHide = $state(false);
	
	onMount(() => {
		// Check if banner should be permanently hidden
		const autoHideEnabled = localStorage.getItem('installationBannerAutoHide');
		if (autoHideEnabled === 'true') {
			autoHide = true;
			showBanner = false;
		}
	});
	
	// Reactive effect to handle autoHide changes
	$effect(() => {
		if (autoHide) {
			localStorage.setItem('installationBannerAutoHide', 'true');
			showBanner = false;
		} else if (typeof autoHide === 'boolean' && localStorage.getItem('installationBannerAutoHide') === 'true') {
			localStorage.removeItem('installationBannerAutoHide');
			showBanner = true;
		}
	});
	
	function closeBanner() {
		// Temporary close - just hide for this session
		showBanner = false;
	}
</script>

{#if showBanner}
<div class="mx-2 sm:mx-4 my-2 rounded-lg border border-blue-600 bg-blue-400/20 p-3 sm:p-4 relative">
	<div class="flex flex-col sm:flex-row items-center sm:items-start gap-3">
		<!-- Close Button -->
		<button 
			class="absolute top-2 right-2 text-gray-400 hover:text-white text-xl font-bold p-1 z-10"
			onclick={closeBanner}
			title="Close banner"
		>
			×
		</button>
		
		<!-- Info Icon -->
		<div class="flex-shrink-0 self-center sm:self-start sm:mt-0.5">
			<svg class="h-8 w-8 sm:h-10 sm:w-10 text-blue-400" fill="currentColor" viewBox="0 0 20 20">
				<path
					fill-rule="evenodd"
					d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zm-7-4a1 1 0 11-2 0 1 1 0 012 0zM9 9a.75.75 0 000 1.5h.253a.25.25 0 01.244.304l-.459 2.066A1.75 1.75 0 0010.747 15H11a.75.75 0 000-1.5h-.253a.25.25 0 01-.244-.304l.459-2.066A1.75 1.75 0 009.253 9H9z"
					clip-rule="evenodd"
				/>
			</svg>
		</div>

		<!-- Content -->
		<div class="flex-grow pr-0 sm:pr-8">
			<h3 class="mb-2 text-xl sm:text-2xl font-bold tracking-wide text-blue-400 text-center sm:text-left">INSTALLATION</h3>

			<p class="mb-3 text-base sm:text-lg text-slate-300 text-center sm:text-left leading-relaxed">
				To install apps or themes, put files on the SD card/LittleFS:
			</p>

			<ul class="text-sm sm:text-md space-y-2 text-slate-300 mb-3 text-left">
				<li class="flex items-start gap-2 leading-relaxed">
					<span class="h-1.5 w-1.5 rounded-full bg-blue-400 mt-2 flex-shrink-0"></span>
					<span><strong>Apps:</strong> Put into <code class="bg-gray-700 px-1 rounded text-blue-300">/BruceJS/&lt;CategoryName&gt;/File.js</code></span>
				</li>
				<li class="flex items-start gap-2 leading-relaxed">
					<span class="h-1.5 w-1.5 rounded-full bg-blue-400 mt-2 flex-shrink-0"></span>
					<span><strong>Themes:</strong> Put into <code class="bg-gray-700 px-1 rounded text-blue-300">/Themes/&lt;ThemeName&gt;/</code></span>
				</li>
			</ul>
			
			<!-- Auto-hide option -->
			<div class="border-t border-blue-500/30 pt-3 mt-3">
				<label class="flex items-center gap-2 text-xs sm:text-sm text-slate-300 cursor-pointer">
					<input 
						type="checkbox" 
						bind:checked={autoHide}
						class="w-4 h-4 text-blue-600 bg-gray-700 border-gray-600 rounded focus:ring-blue-500 focus:ring-2 cursor-pointer"
					>
					<span>Don't show this banner again</span>
				</label>
			</div>
		</div>
	</div>
</div>
{/if}