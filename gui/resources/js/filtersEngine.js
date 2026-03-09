class FiltersEngine{

	constructor(){
		this.filters = {};
		this.result_filtered = [];
	}
	
	_filterType(){
		if(this.filters.onlyType === 'all'){return};
		this.result_filtered = this.result_filtered.filter((item) => {
			return item.type === this.filters.onlyType;
		})
	}
	_filterExclude(){
		if(this.filters.exclude.length === 0){return};
		const regExps = this.filters.exclude.map((excludeItem) => {
			return new RegExp(excludeItem, 'i');
		})
		this.result_filtered = this.result_filtered.filter((checkItem) => {
			for(let i = 0; i < regExps.length; i++){
				if(regExps[i].test(checkItem.path)){return false;}
			}
			return true;
		})
	}
	_filterInclude(){
		if(this.filters.include.length === 0){return};
		const regExps = this.filters.include.map((includeItem) => {
			return new RegExp(includeItem, 'i');
		})
		this.result_filtered = this.result_filtered.filter((checkItem) => {
			for(let i = 0; i < regExps.length; i++){
				if(regExps[i].test(checkItem.path)){return true;}
			}
			return false;
		})
	}

	execute(){
		this.filters = Object.assign({}, window.app.filters);
		this.result_filtered = [...window.app.result_sys];
		this._filterType();
		this._filterExclude();
		this._filterInclude();
		window.app.updateFilteredResult(this.result_filtered);
	}

}
window.filtersEngine = new FiltersEngine();