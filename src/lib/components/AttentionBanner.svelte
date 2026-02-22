<script>
	import { onMount } from 'svelte';
	
	let showBanner = $state(true);
	let autoHide = $state(false);
	
	onMount(() => {
		// Check if banner should be permanently hidden
		const autoHideEnabled = localStorage.getItem('attentionBannerAutoHide');
		if (autoHideEnabled === 'true') {
			autoHide = true;
			showBanner = false;
		}
	});
	
	// Reactive effect to handle autoHide changes
	$effect(() => {
		if (autoHide) {
			localStorage.setItem('attentionBannerAutoHide', 'true');
			showBanner = false;
		} else if (typeof autoHide === 'boolean' && localStorage.getItem('attentionBannerAutoHide') === 'true') {
			localStorage.removeItem('attentionBannerAutoHide');
			showBanner = true;
		}
	});
	
	function closeBanner() {
		// Temporary close - just hide for this session
		showBanner = false;
	}
</script>

{#if showBanner}
<div class="mx-2 sm:mx-4 my-2 rounded-lg border border-purple-600 bg-purple-400/20 p-3 sm:p-4 relative">
	<div class="flex flex-col sm:flex-row items-center sm:items-start gap-3">
		<!-- Close Button -->
		<button 
			class="absolute top-2 right-2 text-gray-400 hover:text-white text-xl font-bold p-1 z-10"
			onclick={closeBanner}
			title="Close banner"
		>
			×
		</button>
		
		<!-- Warning Icon -->
		<div class="flex-shrink-0 self-center sm:self-start sm:mt-0.5">
			<svg class="h-8 w-8 sm:h-10 sm:w-10 text-yellow-400" fill="currentColor" viewBox="0 0 20 20">
				<path
					fill-rule="evenodd"
					d="M8.485 2.495c.673-1.167 2.357-1.167 3.03 0l6.28 10.875c.673 1.167-.17 2.625-1.516 2.625H3.72c-1.347 0-2.19-1.458-1.515-2.625L8.485 2.495zM10 5a.75.75 0 01.75.75v3.5a.75.75 0 01-1.5 0v-3.5A.75.75 0 0110 5zm0 9a1 1 0 100-2 1 1 0 000 2z"
					clip-rule="evenodd"
				/>
			</svg>
		</div>

		<!-- Content -->
		<div class="flex-grow pr-0 sm:pr-8">
			<h3 class="mb-2 text-xl sm:text-2xl font-bold tracking-wide text-yellow-400 text-center sm:text-left">ATTENTION</h3>

			<p class="mb-3 text-base sm:text-lg text-slate-300 text-center sm:text-left leading-relaxed">
				Only trust open-source scripts you can verify! Bruce is open-source under AGPL License - never pay for scripts, firmware forks or themes.
			</p>

			<ul class="text-sm sm:text-md space-y-2 text-slate-300 mb-3 text-left">
				<li class="flex items-start gap-2 leading-relaxed">
					<span class="h-1.5 w-1.5 rounded-full bg-slate-400 mt-2 flex-shrink-0"></span>
					<span>Always read the code before executing</span>
				</li>
				<li class="flex items-start gap-2 leading-relaxed">
					<span class="h-1.5 w-1.5 rounded-full bg-slate-400 mt-2 flex-shrink-0"></span>
					<span>Watch for scams selling "premium" scripts</span>
				</li>
				<li class="flex items-start gap-2 leading-relaxed">
					<span class="h-1.5 w-1.5 rounded-full bg-slate-400 mt-2 flex-shrink-0"></span>
					<span>Official resources only via Bruce App Store</span>
				</li>
				<li class="flex items-start gap-2 leading-relaxed">
					<span class="h-1.5 w-1.5 rounded-full bg-slate-400 mt-2 flex-shrink-0"></span>
					<span>Report suspicious activity to our moderators</span>
				</li>
			</ul>
			
			<!-- Auto-hide option -->
			<div class="border-t border-purple-500/30 pt-3 mt-3">
				<label class="flex items-center gap-2 text-xs sm:text-sm text-slate-300 cursor-pointer">
					<input 
						type="checkbox" 
						bind:checked={autoHide}
						class="w-4 h-4 text-purple-600 bg-gray-700 border-gray-600 rounded focus:ring-purple-500 focus:ring-2 cursor-pointer"
					>
					<span>Don't show this banner again</span>
				</label>
			</div>
		</div>
	</div>
</div>
{/if}
