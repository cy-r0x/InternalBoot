export namespace disk {
	
	export class Partition {
	    index: number;
	    driveLetter: string;
	    label: string;
	    fileSystem: string;
	    sizeBytes: number;
	    sizeGB: string;
	    isBootable: boolean;
	    type: string;
	
	    static createFrom(source: any = {}) {
	        return new Partition(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.index = source["index"];
	        this.driveLetter = source["driveLetter"];
	        this.label = source["label"];
	        this.fileSystem = source["fileSystem"];
	        this.sizeBytes = source["sizeBytes"];
	        this.sizeGB = source["sizeGB"];
	        this.isBootable = source["isBootable"];
	        this.type = source["type"];
	    }
	}

}

export namespace iso {
	
	export class Info {
	    path: string;
	    sizeBytes: number;
	    sizeGB: string;
	    isValid: boolean;
	    version: string;
	    mountPoint: string;
	
	    static createFrom(source: any = {}) {
	        return new Info(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.path = source["path"];
	        this.sizeBytes = source["sizeBytes"];
	        this.sizeGB = source["sizeGB"];
	        this.isValid = source["isValid"];
	        this.version = source["version"];
	        this.mountPoint = source["mountPoint"];
	    }
	}

}

export namespace main {
	
	export class InstallRequest {
	    isoPath: string;
	    driveLetter: string;
	    targetPath: string;
	    setAsDefault: boolean;
	
	    static createFrom(source: any = {}) {
	        return new InstallRequest(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.isoPath = source["isoPath"];
	        this.driveLetter = source["driveLetter"];
	        this.targetPath = source["targetPath"];
	        this.setAsDefault = source["setAsDefault"];
	    }
	}
	export class Progress {
	    stage: string;
	    message: string;
	    percent: number;
	    isRunning: boolean;
	    hasError: boolean;
	    errorMsg: string;
	    canReboot: boolean;
	    canCleanup: boolean;
	
	    static createFrom(source: any = {}) {
	        return new Progress(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.stage = source["stage"];
	        this.message = source["message"];
	        this.percent = source["percent"];
	        this.isRunning = source["isRunning"];
	        this.hasError = source["hasError"];
	        this.errorMsg = source["errorMsg"];
	        this.canReboot = source["canReboot"];
	        this.canCleanup = source["canCleanup"];
	    }
	}

}

